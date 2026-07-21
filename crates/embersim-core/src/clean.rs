use anyhow::Result;
use regex::Regex;

pub fn normalize(preprocessed: &str) -> Result<Vec<String>> {
    let re_directive = Regex::new(r#"(?m)^\s*#\s*\d+.*$"#)?;
    let no_directives = re_directive.replace_all(preprocessed, "");

    let re_attr = Regex::new(r"__attribute__\s*\(\([^)]*\)\)")?;
    let no_attr = re_attr.replace_all(&no_directives, "");

    let re_keywords = Regex::new(
        r"\b(__weak|__WEAK|inline|__inline|__STATIC_INLINE|static|extern)\b"
    )?;
    let cleaned = re_keywords.replace_all(&no_attr, "");

    // Split on newlines first (handles #define, typedef, struct, enum),
    // then split each line on semicolons (handles multiple decls per line).
    let mut result = Vec::new();
    for line in cleaned.lines() {
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        for segment in line.split(';') {
            let segment = segment.trim().to_string();
            if !segment.is_empty() {
                result.push(segment);
            }
        }
    }

    Ok(result)
}