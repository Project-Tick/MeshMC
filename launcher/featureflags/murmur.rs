/* SPDX-FileCopyrightText: 2026 Project Tick
 * SPDX-FileContributor: Project Tick
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Project Tick
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! MurmurHash3 (x86 32-bit), the hash Unleash uses for rollout stickiness.
//!
//! Unleash normalises a value into the range 1..=100 with
//! `murmurhash3_32(group:id) % 100 + 1` and compares it against the configured
//! rollout percentage. We must match that exactly or stickiness breaks across
//! the C++/Rust boundary and against the server's own bucketing.

const C1: u32 = 0xcc9e_2d51;
const C2: u32 = 0x1b87_3593;

/// MurmurHash3 x86_32 with seed 0, matching the reference implementation used
/// by the Unleash SDKs.
pub fn murmurhash3_x86_32(data: &[u8], seed: u32) -> u32 {
    let mut h1 = seed;
    let nblocks = data.len() / 4;

    // body
    for i in 0..nblocks {
        let k = i * 4;
        let mut k1 = u32::from_le_bytes([data[k], data[k + 1], data[k + 2], data[k + 3]]);
        k1 = k1.wrapping_mul(C1);
        k1 = k1.rotate_left(15);
        k1 = k1.wrapping_mul(C2);

        h1 ^= k1;
        h1 = h1.rotate_left(13);
        h1 = h1.wrapping_mul(5).wrapping_add(0xe654_6b64);
    }

    // tail
    let tail = &data[nblocks * 4..];
    let mut k1: u32 = 0;
    if tail.len() >= 3 {
        k1 ^= (tail[2] as u32) << 16;
    }
    if tail.len() >= 2 {
        k1 ^= (tail[1] as u32) << 8;
    }
    if !tail.is_empty() {
        k1 ^= tail[0] as u32;
        k1 = k1.wrapping_mul(C1);
        k1 = k1.rotate_left(15);
        k1 = k1.wrapping_mul(C2);
        h1 ^= k1;
    }

    // finalization
    h1 ^= data.len() as u32;
    h1 ^= h1 >> 16;
    h1 = h1.wrapping_mul(0x85eb_ca6b);
    h1 ^= h1 >> 13;
    h1 = h1.wrapping_mul(0xc2b2_ae35);
    h1 ^= h1 >> 16;
    h1
}

/// Unleash normalisation: maps "group:id" into the inclusive range 1..=100.
pub fn normalized_value(id: &str, group_id: &str) -> u32 {
    let payload = format!("{group_id}:{id}");
    murmurhash3_x86_32(payload.as_bytes(), 0) % 100 + 1
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn known_murmur_vectors() {
        // Reference vectors for MurmurHash3 x86_32, seed 0.
        assert_eq!(murmurhash3_x86_32(b"", 0), 0);
        assert_eq!(murmurhash3_x86_32(b"hello", 0), 0x248b_fa47);
    }

    #[test]
    fn normalized_in_range() {
        for i in 0..1000 {
            let v = normalized_value(&format!("user{i}"), "my-toggle");
            assert!((1..=100).contains(&v), "value {v} out of range");
        }
    }

    #[test]
    fn normalized_is_stable() {
        let a = normalized_value("123", "demo");
        let b = normalized_value("123", "demo");
        assert_eq!(a, b);
    }
}
