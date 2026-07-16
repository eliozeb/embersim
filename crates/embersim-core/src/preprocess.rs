use anyhow::{Context, Result};
use std::path::Path;
use std::process::Command;

/// Try to find a working C preprocessor: gcc first, then clang.
fn find_preprocessor() -> Result<String> {
    for cmd in &["gcc", "clang"] {
        if Command::new(cmd).arg("--version").output().is_ok() {
            return Ok(cmd.to_string());
        }
    }
    anyhow::bail!(
        "No C preprocessor found. Please install one of the following:\n\
         \n\
         - MinGW-w64 (gcc): https://www.msys2.org/  (run: pacman -S mingw-w64-ucrt-x86_64-gcc)\n\
         - LLVM (clang):    https://releases.llvm.org/\n\
         \n\
         Then ensure it is on your PATH and try again."
    )
}

pub fn run(
    header_path: &Path,
    include_paths: &[String],
    defines: &[String],
) -> Result<String> {
    let preprocessor = find_preprocessor()?;
    
    let mut cmd = Command::new(&preprocessor);
    cmd.arg("-E")
        .arg("-dD")
        .arg("-DEMBERSIM_MOCK");

    for d in defines {
        cmd.arg(format!("-D{}", d));
    }

    for inc in include_paths {
        cmd.arg("-I").arg(inc);
    }

    cmd.arg(header_path);

    let output = cmd
        .output()
        .with_context(|| format!("Failed to run `{} -E`", preprocessor))?;

    if !output.status.success() {
        let stderr = String::from_utf8_lossy(&output.stderr);
        anyhow::bail!(
            "{} -E failed with exit code {:?}:\n{}",
            preprocessor,
            output.status.code(),
            stderr
        );
    }

    Ok(String::from_utf8_lossy(&output.stdout).to_string())
}