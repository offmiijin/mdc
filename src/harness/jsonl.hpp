// SPDX-License-Identifier: AGPL-3.0-only
// SPDX-FileCopyrightText: 2026 mdc contributors
//
// Primitivas de escrita da transcrição JSONL, compartilhadas por todos os
// harnesses. O formato é o de docs/HARNESS.md §3: doubles como 16 dígitos hex do
// padrão IEEE-754, campos em ordem fixa, nada de locale.
#pragma once

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "mdc/core/aabb.hpp"
#include "mdc/core/vec3.hpp"

namespace mdc::harness {

/// Um double, entre aspas, em bits. Bits e não decimal: Java e C++ formatam
/// hex-float de formas diferentes, e a comparação viraria discussão de formato.
inline void print_bits(double value) {
    std::uint64_t raw = 0;
    std::memcpy(&raw, &value, sizeof raw);
    std::printf("\"%016" PRIx64 "\"", raw);
}

inline void print_vec(const core::Vec3& v) {
    std::printf("[");
    print_bits(v.x);
    std::printf(",");
    print_bits(v.y);
    std::printf(",");
    print_bits(v.z);
    std::printf("]");
}

inline void print_box(const core::Aabb& b) {
    std::printf("[");
    print_bits(b.min_x());
    std::printf(",");
    print_bits(b.min_y());
    std::printf(",");
    print_bits(b.min_z());
    std::printf(",");
    print_bits(b.max_x());
    std::printf(",");
    print_bits(b.max_y());
    std::printf(",");
    print_bits(b.max_z());
    std::printf("]");
}

/// Cabeçalho de proveniência, idêntico nos dois lados.
inline void print_header(const char* harness) {
    std::printf("{\"schema\":1,\"harness\":\"%s\"}\n", harness);
}

} // namespace mdc::harness
