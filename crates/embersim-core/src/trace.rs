// trace.rs — trace comparison for regression detection
use anyhow::{Context, Result};
use std::path::Path;

#[derive(Debug, Clone)]
pub struct TraceEvent {
    pub func: String,
    pub peripheral: String,
    pub raw: String,
}

#[derive(Debug)]
pub struct Divergence {
    pub line_number: usize,
    pub expected_line: String,
    pub observed_line: String,
    pub expected_func: String,
    pub observed_func: String,
}

/// Parse a JSON Lines trace file. Lines that are not valid JSON are kept as raw.
pub fn parse_trace(path: &Path) -> Result<Vec<TraceEvent>> {
    let content = std::fs::read_to_string(path)
        .with_context(|| format!("Cannot read trace file: {}", path.display()))?;
    let mut events = Vec::new();
    for line in content.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        // Try to extract func and peripheral from the JSON line
        let func = extract_field(line, "func");
        let peripheral = extract_field(line, "peripheral");
        events.push(TraceEvent {
            func: func.unwrap_or_else(|| "(unknown)".into()),
            peripheral: peripheral.unwrap_or_else(|| "(unknown)".into()),
            raw: line.to_string(),
        });
    }
    Ok(events)
}

/// Extract a JSON field value from a line by scanning for "field":"value" or "field":value
fn extract_field(line: &str, field: &str) -> Option<String> {
    let pattern = format!("\"{}\":", field);
    let start = line.find(&pattern)? + pattern.len();
    let rest = &line[start..];
    if rest.starts_with('"') {
        // String value: find closing quote
        let val_start = 1;
        let val_end = rest[1..].find('"')? + 1;
        Some(rest[val_start..val_end].to_string())
    } else {
        // Non-string value: find comma, brace, or end
        let val_end = rest.find(|c: char| c == ',' || c == '}' || c == ']')
            .unwrap_or(rest.len());
        Some(rest[..val_end].trim().to_string())
    }
}

/// Find the first line where two traces differ. Returns the divergence if found.
pub fn find_first_divergence(
    golden: &[TraceEvent],
    candidate: &[TraceEvent],
) -> Option<Divergence> {
    let min_len = golden.len().min(candidate.len());
    for i in 0..min_len {
        if golden[i].raw != candidate[i].raw {
            return Some(Divergence {
                line_number: i + 1,
                expected_line: golden[i].raw.clone(),
                observed_line: candidate[i].raw.clone(),
                expected_func: golden[i].func.clone(),
                observed_func: candidate[i].func.clone(),
            });
        }
    }
    // Length mismatch
    if golden.len() != candidate.len() {
        let (_label, line) = if golden.len() > candidate.len() {
            ("(end of candidate trace)", &golden[candidate.len()])
        } else {
            ("(end of golden trace)", &candidate[golden.len()])
        };
        return Some(Divergence {
            line_number: min_len + 1,
            expected_line: if golden.len() > candidate.len() { line.raw.clone() } else { "(none)".into() },
            observed_line: if golden.len() > candidate.len() { "(none)".into() } else { line.raw.clone() },
            expected_func: if golden.len() > candidate.len() { line.func.clone() } else { "(end)".into() },
            observed_func: if golden.len() > candidate.len() { "(end)".into() } else { line.func.clone() },
        });
    }
    None
}

pub fn evidence_strength(divergence_reproduced: bool, deterministic: bool) -> &'static str {
    if divergence_reproduced && deterministic {
        "HIGH — same divergence reproduced across multiple deterministic runs"
    } else if divergence_reproduced {
        "MEDIUM — divergence found, but execution contains nondeterministic regions"
    } else {
        "LOW — trace mismatch detected but ordering/timing ambiguity exists"
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn make_event(func: &str, peripheral: &str) -> TraceEvent {
        TraceEvent {
            func: func.into(),
            peripheral: peripheral.into(),
            raw: format!(r#"{{"func":"{}","peripheral":"{}"}}"#, func, peripheral),
        }
    }

    #[test]
    fn test_identical_traces() {
        let g = vec![make_event("A", "TIM"), make_event("B", "UART")];
        let c = g.clone();
        assert!(find_first_divergence(&g, &c).is_none());
    }

    #[test]
    fn test_single_event_changed() {
        let golden = vec![
            make_event("A", "TIM"),
            make_event("B", "UART"),
            make_event("C", "TIM"),
        ];
        let buggy = vec![
            make_event("A", "TIM"),
            make_event("B", "UART"),
            make_event("X", "TIM"),  // changed
        ];
        let div = find_first_divergence(&golden, &buggy).unwrap();
        assert_eq!(div.line_number, 3);
        assert_eq!(div.expected_func, "C");
        assert_eq!(div.observed_func, "X");
    }

    #[test]
    fn test_single_event_removed() {
        let golden = vec![
            make_event("A", "TIM"),
            make_event("B", "UART"),
            make_event("C", "TIM"),
        ];
        let buggy = vec![
            make_event("A", "TIM"),
            make_event("C", "TIM"),  // B removed
        ];
        let div = find_first_divergence(&golden, &buggy).unwrap();
        assert_eq!(div.line_number, 2);
        assert_eq!(div.expected_func, "B");
        assert_eq!(div.observed_func, "C");
    }

    #[test]
    fn test_truncated_trace() {
        let golden = vec![make_event("A", "TIM"), make_event("B", "UART")];
        let buggy = vec![make_event("A", "TIM")];
        let div = find_first_divergence(&golden, &buggy).unwrap();
        assert_eq!(div.line_number, 2);
    }

    #[test]
    fn test_field_extraction() {
        assert_eq!(
            extract_field(r#"{"ts_ms":0,"func":"HAL_UART_Transmit","peripheral":"UART"}"#, "func"),
            Some("HAL_UART_Transmit".into())
        );
        assert_eq!(
            extract_field(r#"{"ts_ms":5,"peripheral":"runtime","func":"NVIC_DISPATCH",{"irq":28}}"#, "func"),
            Some("NVIC_DISPATCH".into())
        );
    }
}
