//! Optional standalone map validator. Not linked into the engine yet.
use std::{env, fs, process};
fn main() {
    let path = env::args().nth(1).unwrap_or_else(|| "assets/maps/map.txt".to_string());
    let contents = fs::read_to_string(&path).unwrap_or_else(|e| {
        eprintln!("Could not read {path}: {e}"); process::exit(1);
    });
    let rows: Vec<&str> = contents.lines().filter(|s| !s.trim().is_empty()).collect();
    let width = rows.first().map_or(0, |s| s.len());
    let valid = rows.len() >= 3 && width >= 3 && rows.iter().all(|row| {
        row.len() == width && row.bytes().all(|b| b == b'0' || b == b'1')
            && row.starts_with('1') && row.ends_with('1')
    }) && rows.first().unwrap().bytes().all(|b| b == b'1')
      && rows.last().unwrap().bytes().all(|b| b == b'1');
    if !valid { eprintln!("Invalid map: {path}"); process::exit(1); }
    println!("Valid Helios map: {} rows x {} columns", rows.len(), width);
}
