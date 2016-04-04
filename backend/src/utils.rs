use std::collections::HashMap;
use std::iter::FromIterator;
use uuid;
use error::RequestError;

#[inline(always)]
fn from_hex(byte: u8) -> Option<u8> {
    match byte {
        b'0'...b'9' => Some(byte - b'0'),
        b'A'...b'F' => Some(byte - b'A' + 10),
        b'a'...b'f' => Some(byte - b'a' + 10),
        _ => None
    }
}

fn percent_decode(s: &str) -> Vec<u8> {
    let mut i = 0;
    let mut out = Vec::<u8>::new();
    let input = (*s).as_bytes();
    while i < input.len() {
        let c = input[i];
        if c == b'%' && i + 2 < input.len() {
            if let (Some(h), Some(l)) = (from_hex(input[i + 1]), from_hex(input[i + 2])) {
                out.push(h * 0x10 + l);
                i += 3;
                continue
            }
        }

        out.push(c);
        i += 1;
    }
    out
}

pub fn parse_post_raw(orig: &str) -> HashMap<Vec<u8>, Vec<u8>> {
    HashMap::from_iter(
        orig.split('&')
        .filter_map(|i| {
            let vec = i.split('=').collect::<Vec<_>>();
            match vec.len() {
                2 => Some((percent_decode(vec[0]), percent_decode(vec[1]))),
                _ => None
            }
        }))
}

pub fn parse_post(orig: &str) -> Result<HashMap<String, String>, RequestError> {
    let vals: Result<Vec<_>, RequestError> = parse_post_raw(&orig)
        .into_iter()
        .map(|ent| {
            if let (Ok(k), Ok(v)) = (String::from_utf8(ent.0), String::from_utf8(ent.1)) {
                Ok((k, v))
            } else {
                Err(RequestError::BadRequest.into())
            }
        }).collect();
    Ok(HashMap::from_iter(vals?))
}

pub fn uuid() -> String {
    uuid::Uuid::new_v4().to_simple_string()
}

#[macro_export]
macro_rules! stry {
    ($e:expr) => (match $e {
        Ok(e) => e,
        Err(e) => return Err(e.to_string())
    })
}
