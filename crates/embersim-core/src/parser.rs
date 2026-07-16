use anyhow::Result;
use regex::Regex;
use crate::model::{HalFunction, Parameter};

pub fn extract(declarations: &[String]) -> Result<Vec<HalFunction>> {
    let re_func = Regex::new(
        r"^\s*([A-Za-z0-9_\s\*]+?)\s+(HAL_[A-Za-z0-9_]+)\s*\((.*)\)\s*$"
    )?;

    let mut functions = Vec::new();

    for decl in declarations {
        let decl = decl.trim();
        if decl.is_empty() {
            continue;
        }

        if decl.starts_with("typedef")
            || decl.starts_with("struct")
            || decl.starts_with("enum")
            || decl.starts_with("union")
            || decl.starts_with("#define")
        {
            continue;
        }

        if let Some(cap) = re_func.captures(decl) {
            let return_type = cap[1].trim().to_string();
            let name = cap[2].to_string();
            let params_raw = cap[3].trim().to_string();

            if return_type.contains('(') || return_type.contains("typedef") {
                continue;
            }
            if params_raw.contains('{') {
                continue;
            }

            let params = parse_params(&params_raw)?;

            functions.push(HalFunction {
                return_type,
                name,
                params,
            });
        }
    }

    Ok(functions)
}

fn parse_params(raw: &str) -> Result<Vec<Parameter>> {
    if raw == "void" || raw.trim().is_empty() {
        return Ok(Vec::new());
    }

    let mut params = Vec::new();

    for fragment in raw.split(',') {
        let fragment = fragment.trim();
        if fragment.is_empty() {
            continue;
        }

        if fragment == "..." {
            params.push(Parameter {
                type_name: "...".to_string(),
                name: "args".to_string(),
                is_const: false,
                pointer_depth: 0,
            });
            continue;
        }

        let (is_const, rest) = if fragment.starts_with("const ") {
            (true, &fragment[6..])
        } else {
            (false, fragment)
        };

        // Count pointer depth
        let pointer_depth = rest.chars().filter(|&c| c == '*').count();

        // Remove all '*' and trim
        let without_stars: String = rest.chars().filter(|&c| c != '*').collect();
        let trimmed = without_stars.trim();

        // Split by whitespace: last token is the identifier, rest is the type
        let tokens: Vec<&str> = trimmed.split_whitespace().collect();

        if tokens.is_empty() {
            continue;
        }

        let (type_name, name) = if tokens.len() == 1 {
            // Anonymous parameter: only type, no name
            (tokens[0].to_string(), format!("arg{}", params.len()))
        } else {
            let name = tokens.last().unwrap().to_string();
            let type_name = tokens[..tokens.len() - 1].join(" ");
            (type_name, name)
        };

        // Strip HAL-specific qualifiers from type name
        let type_name = type_name
            .replace("__IO", "")
            .replace("volatile", "")
            .trim()
            .to_string();

        params.push(Parameter {
            type_name,
            name,
            is_const,
            pointer_depth,
        });
    }

    Ok(params)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_params_simple() {
        let p = parse_params("GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin").unwrap();
        assert_eq!(p.len(), 2);
        assert_eq!(p[0].type_name, "GPIO_TypeDef");
        assert_eq!(p[0].name, "GPIOx");
        assert_eq!(p[0].pointer_depth, 1);
        assert!(!p[0].is_const);

        assert_eq!(p[1].type_name, "uint32_t");
        assert_eq!(p[1].name, "GPIO_Pin");
        assert_eq!(p[1].pointer_depth, 0);
    }

    #[test]
    fn test_parse_params_const_pointer() {
        let p = parse_params("const uint8_t *pData, uint16_t Size, uint32_t Timeout").unwrap();
        assert_eq!(p.len(), 3);
        assert_eq!(p[0].type_name, "uint8_t");
        assert_eq!(p[0].name, "pData");
        assert_eq!(p[0].pointer_depth, 1);
        assert!(p[0].is_const);

        assert_eq!(p[1].type_name, "uint16_t");
        assert_eq!(p[1].name, "Size");
        assert_eq!(p[1].pointer_depth, 0);

        assert_eq!(p[2].type_name, "uint32_t");
        assert_eq!(p[2].name, "Timeout");
        assert_eq!(p[2].pointer_depth, 0);
    }

    #[test]
    fn test_parse_params_void() {
        let p = parse_params("void").unwrap();
        assert!(p.is_empty());
    }

    #[test]
    fn test_parse_params_anonymous() {
        let p = parse_params("int").unwrap();
        assert_eq!(p.len(), 1);
        assert_eq!(p[0].type_name, "int");
        assert_eq!(p[0].name, "arg0");
        assert_eq!(p[0].pointer_depth, 0);
    }

    #[test]
    fn test_extract_from_declarations() {
        let decls = vec![
            "HAL_StatusTypeDef HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)".to_string(),
            "void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)".to_string(),
            "typedef enum { HAL_OK = 0 } HAL_StatusTypeDef;".to_string(),
        ];

        let funcs = extract(&decls).unwrap();
        assert_eq!(funcs.len(), 2);
        assert!(funcs.iter().any(|f| f.name == "HAL_GPIO_Init"));
        assert!(funcs.iter().any(|f| f.name == "HAL_GPIO_WritePin"));
    }
}