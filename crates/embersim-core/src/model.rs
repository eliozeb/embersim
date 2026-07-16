use serde::{Deserialize, Serialize};
use std::path::PathBuf;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Parameter {
    pub type_name: String,
    pub name: String,
    pub is_const: bool,
    pub pointer_depth: usize,
}

impl Parameter {
    pub fn to_c_decl(&self) -> String {
        let mut s = String::new();
        if self.is_const { s.push_str("const "); }
        s.push_str(&self.type_name);
        for _ in 0..self.pointer_depth { s.push('*'); }
        s.push(' ');
        s.push_str(&self.name);
        s
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct HalFunction {
    pub return_type: String,
    pub name: String,
    pub params: Vec<Parameter>,
}

impl HalFunction {
    pub fn to_c_decl(&self) -> String {
        let param_list: Vec<String> = self.params.iter().map(|p| p.to_c_decl()).collect();
        format!("{} {}({})", self.return_type, self.name, param_list.join(", "))
    }
}

#[derive(Debug, Clone, Default)]
pub struct Project {
    pub name: String,
    pub root: PathBuf,
    pub hal_header: Option<PathBuf>,
    pub include_paths: Vec<String>,
    pub source_files: Vec<PathBuf>,
}

impl Project {
    pub fn new(root: PathBuf) -> Self {
        Self {
            name: root.file_name()
                .map(|n| n.to_string_lossy().to_string())
                .unwrap_or_else(|| "untitled".to_string()),
            root,
            ..Default::default()
        }
    }
}