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

    let normalized = cleaned.replace('\n', " ");

    Ok(normalized
        .split(';')
        .map(|s| s.trim().to_string())
        .filter(|s| !s.is_empty())
        .collect())
}