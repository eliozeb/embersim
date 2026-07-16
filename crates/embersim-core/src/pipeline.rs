use anyhow::Result;
use std::path::Path;
use crate::model::HalFunction;
use crate::{clean, parser, preprocess};

pub fn run(
    header_path: &Path,
    include_paths: &[String],
    defines: &[String],
) -> Result<Vec<HalFunction>> {
    let raw = preprocess::run(header_path, include_paths, defines)?;
    let declarations = clean::normalize(&raw)?;
    let functions = parser::extract(&declarations)?;
    Ok(deduplicate(functions))
}

fn deduplicate(functions: Vec<HalFunction>) -> Vec<HalFunction> {
    let mut seen = std::collections::HashSet::new();
    let mut out = Vec::new();
    for f in functions {
        if seen.insert(f.name.clone()) { out.push(f); }
    }
    out
}