use serde::{Deserialize, Serialize};

// --------------------- EmberSim Project Config (embersim.toml) ---------------------

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct EmberSimConfig {
    pub project: ProjectMeta,
    #[serde(default)]
    pub build: BuildConfig,
    #[serde(default)]
    pub simulation: SimulationConfig,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ProjectMeta {
    pub name: String,
    #[serde(default)]
    pub description: String,
}

#[derive(Debug, Serialize, Deserialize, Clone, Default)]
pub struct BuildConfig {
    #[serde(default)]
    pub sources: Vec<String>,
    #[serde(default)]
    pub includes: Vec<String>,
    #[serde(default)]
    pub defines: Vec<String>,
    #[serde(default)]
    pub hal_header: String,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct SimulationConfig {
    #[serde(default = "default_entry")]
    pub entry: String,
    #[serde(default = "default_trace")]
    pub trace: String,
    #[serde(default = "default_duration_ms")]
    pub duration_ms: u64,
}

fn default_entry() -> String { "app_init".into() }
fn default_trace() -> String { "trace.jsonl".into() }
fn default_duration_ms() -> u64 { 1000 }

impl Default for SimulationConfig {
    fn default() -> Self {
        Self {
            entry: default_entry(),
            trace: default_trace(),
            duration_ms: default_duration_ms(),
        }
    }
}

// --------------------- HAL Function Models ---------------------

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

// --------------------- Project Config (embersim.json) ---------------------
// ONLY ONE ProjectConfig and ONE Project definition below.

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct ProjectConfig {
    pub project: Project,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct Project {
    pub name: String,
    pub source_files: Vec<String>,   // paths relative to project root
    #[serde(default)]
    pub exclude_files: Vec<String>,
    #[serde(default)]
    pub hal_path: String,
    #[serde(default)]
    pub hal_conf: String,
}