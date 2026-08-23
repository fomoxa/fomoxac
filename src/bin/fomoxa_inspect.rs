use std::process::ExitCode;

use fomoxac::inspect;

fn main() -> ExitCode {
    let options = match inspect::parse(std::env::args_os().skip(1)) {
        Ok(Some(options)) => options,
        Ok(None) => {
            print!("{}", inspect::USAGE);
            return ExitCode::SUCCESS;
        }
        Err(message) => {
            eprintln!("fomoxa-inspect: {message}\n\n{}", inspect::USAGE);
            return ExitCode::from(2);
        }
    };

    match inspect::run(&options) {
        Ok(report) => {
            print!("{report}");
            ExitCode::SUCCESS
        }
        Err(message) => {
            eprintln!("fomoxa-inspect: {message}");
            ExitCode::FAILURE
        }
    }
}
