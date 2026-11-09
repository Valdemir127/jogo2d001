#include "sim_core.h"
#include "material_table.h"

#include <algorithm>
#include <cstring>

SimCore::SimCore(int largura, int altura, uint32_t seed)
    : m_largura(largura), m_altura(altura), m_seed_base(seed) {
    m_chunks_x = (largura + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_chunks_y = (altura + CHUNK_SIZE - 1) / CHUNK_SIZE;
    m_chunks.resize(static_cast<size_t>(m_chunks_x) * static_cast<size_t>(m_chunks_y));
    for (auto& chunk : m_chunks) {
        chunk.dirty_rect.x0 = 0; chunk.dirty_rect.y0 = 0;
        chunk.dirty_rect.x1 = CHUNK_SIZE - 1; chunk.dirty_rect.y1 = CHUNK_SIZE - 1;
    }
}

void SimCore::set_cell(int x, int y, uint16_t mat_id, uint8_t temp_vida) {
    if (!dentro(x, y)) return;
    int cx = x / CHUNK_SIZE;
    int cy = y / CHUNK_SIZE;
    int lx = x % CHUNK_SIZE;
    int ly = y % CHUNK_SIZE;
    Chunk& chunk = m_chunks[cy * m_chunks_x + cx];
    int idx_c = ly * CHUNK_SIZE + lx;
    chunk.mat_id[idx_c] = mat_id;
    chunk.temp_vida[idx_c] = temp_vida;
    chunk.flags[idx_c] = 0;
    marcar_dirty(x, y);
}

void SimCore::marcar_dirty(int x, int y) {
    int cx = x / CHUNK_SIZE;
    int cy = y / CHUNK_SIZE;
    Chunk& chunk = m_chunks[cy * m_chunks_x + cx];
    chunk.next_dirty_rect.add(x % CHUNK_SIZE, y % CHUNK_SIZE);
    chunk.ativo = true;
    chunk.ticks_parado = 0;

    // Acorda vizinhos se estiver perto da borda
    int lx = x % CHUNK_SIZE;
    int ly = y % CHUNK_SIZE;
    if (lx == 0 && cx > 0) acordar_chunk(cx - 1, cy);
    if (lx == CHUNK_SIZE - 1 && cx < m_chunks_x - 1) acordar_chunk(cx + 1, cy);
    if (ly == 0 && cy > 0) acordar_chunk(cx, cy - 1);
    if (ly == CHUNK_SIZE - 1 && cy < m_chunks_y - 1) acordar_chunk(cx, cy + 1);
}

void SimCore::acordar_chunk(int cx, int cy) {
    Chunk& chunk = m_chunks[cy * m_chunks_x + cx];
    chunk.ativo = true;
    chunk.ticks_parado = 0;
}

void SimCore::paint(int cx_center, int cy_center, int raio, uint16_t mat_id) {
    if (mat_id >= MaterialTable::instancia().total()) return;
    const int r2 = raio * raio;
    for (int y = cy_center - raio; y <= cy_center + raio; ++y) {
        if (y < 0 || y >= m_altura) continue;
        for (int x = cx_center - raio; x <= cx_center + raio; ++x) {
            if (x < 0 || x >= m_largura) continue;
            const int dx = x - cx_center;
            const int dy = y - cy_center;
            if (dx * dx + dy * dy <= r2) {
                set_cell(x, y, mat_id, 0);
            }
        }
    }
}

void SimCore::preencher_regiao(int x0, int y0, int x1, int y1, uint16_t mat_id) {
    for (int y = std::max(0, y0); y < std::min(m_altura, y1); ++y) {
        for (int x = std::max(0, x0); x < std::min(m_largura, x1); ++x) {
            set_cell(x, y, mat_id, 0);
        }
    }
}

void SimCore::preencher_tudo(uint16_t mat_id) {
    for (auto& chunk : m_chunks) {
        std::fill(chunk.mat_id.begin(), chunk.mat_id.end(), mat_id);
        std::fill(chunk.temp_vida.begin(), chunk.temp_vida.end(), 0);
        std::fill(chunk.flags.begin(), chunk.flags.end(), 0);
        chunk.ativo = true;
        chunk.ticks_parado = 0;
        chunk.dirty_rect.x0 = 0; chunk.dirty_rect.y0 = 0;
        chunk.dirty_rect.x1 = CHUNK_SIZE - 1; chunk.dirty_rect.y1 = CHUNK_SIZE - 1;
    }
}

uint32_t SimCore::rng_celula(int x, int y) const {
    uint32_t z = m_seed_base
               + m_tick * 0x9E3779B1u
               + static_cast<uint32_t>(x) * 0x85EBCA77u
               + static_cast<uint32_t>(y) * 0xC2B2AE3Du;
    z = (z ^ (z >> 16)) * 0x85ebca6bU;
    z = (z ^ (z >> 13)) * 0xc2b2ae35U;
    return z ^ (z >> 16);
}

bool SimCore::pode_trocar(uint16_t origem, uint16_t destino) const {
    if (destino == MAT_AR) return true;
    const MaterialTable& t = MaterialTable::instancia();
    const Material& mo = t.get(origem);
    const Material& md = t.get(destino);
    if (md.estado == Estado::Liquid || md.estado == Estado::Gas) {
        return mo.densidade > md.densidade;
    }
    return false;
}

bool SimCore::tentar_mover(int x, int y, int dx, int dy) {
    const int nx = x + dx;
    const int ny = y + dy;
    if (!dentro(nx, ny)) return false;

    const int cx = x / CHUNK_SIZE;
    const int cy = y / CHUNK_SIZE;
    const int lx = x % CHUNK_SIZE;
    const int ly = y % CHUNK_SIZE;
    const int src_chunk_idx = cy * m_chunks_x + cx;
    const int src_cell_idx = ly * CHUNK_SIZE + lx;

    const int ncx = nx / CHUNK_SIZE;
    const int ncy = ny / CHUNK_SIZE;
    const int nlx = nx % CHUNK_SIZE;
    const int nly = ny % CHUNK_SIZE;
    const int dst_chunk_idx = ncy * m_chunks_x + ncx;
    const int dst_cell_idx = nly * CHUNK_SIZE + nlx;

    Chunk& src_chunk = m_chunks[src_chunk_idx];
    Chunk& dst_chunk = m_chunks[dst_chunk_idx];

    if (dst_chunk.flags[dst_cell_idx] & FLAG_MOVIDA) return false;

    const uint16_t origem  = src_chunk.mat_id[src_cell_idx];
    const uint16_t destino = dst_chunk.mat_id[dst_cell_idx];
    if (!pode_trocar(origem, destino)) return false;

    std::swap(src_chunk.mat_id[src_cell_idx], dst_chunk.mat_id[dst_cell_idx]);
    std::swap(src_chunk.temp_vida[src_cell_idx], dst_chunk.temp_vida[dst_cell_idx]);
    src_chunk.flags[src_cell_idx] |= FLAG_MOVIDA;
    dst_chunk.flags[dst_cell_idx] |= FLAG_MOVIDA;

    src_chunk.next_dirty_rect.add(lx, ly);
    dst_chunk.next_dirty_rect.add(nlx, nly);

    src_chunk.ativo = true;
    src_chunk.ticks_parado = 0;
    dst_chunk.ativo = true;
    dst_chunk.ticks_parado = 0;

    return true;
}

void SimCore::atualizar_powder(int x, int y) {
    if (tentar_mover(x, y, 0, 1)) return;
    const uint32_t r = rng_celula(x, y);
    const int dir = (r & 1) ? 1 : -1;
    if (tentar_mover(x, y, dir, 1)) return;
    if (tentar_mover(x, y, -dir, 1)) return;
}

void SimCore::atualizar_liquid(int x, int y) {
    if (tentar_mover(x, y, 0, 1)) return;
    const uint32_t r = rng_celula(x, y);
    const int dir = (r & 1) ? 1 : -1;
    if (tentar_mover(x, y, dir, 1)) return;
    if (tentar_mover(x, y, -dir, 1)) return;
    if (tentar_mover(x, y, dir, 0)) return;
    if (tentar_mover(x, y, -dir, 0)) return;
}

void SimCore::atualizar_celula(int x, int y) {
    const int cx = x / CHUNK_SIZE;
    const int cy = y / CHUNK_SIZE;
    const int lx = x % CHUNK_SIZE;
    const int ly = y % CHUNK_SIZE;
    const int chunk_idx = cy * m_chunks_x + cx;
    const int cell_idx = ly * CHUNK_SIZE + lx;

    const Chunk& chunk = m_chunks[chunk_idx];
    if (chunk.flags[cell_idx] & FLAG_MOVIDA) return;

    const uint16_t mat = chunk.mat_id[cell_idx];
    if (mat == MAT_AR) return;

    const Material& m = MaterialTable::instancia().get(mat);
    switch (m.comportamento) {
        case Comportamento::Powder: atualizar_powder(x, y); break;
        case Comportamento::Liquid: atualizar_liquid(x, y); break;
        case Comportamento::Static: break;
    }
}

void SimCore::step() {
    ++m_tick;
    m_direcao_esq_para_dir = !m_direcao_esq_para_dir;
    m_chunks_ativos_count = 0;

    for (int y = m_altura - 1; y >= 0; --y) {
        if (m_direcao_esq_para_dir) {
            for (int x = 0; x < m_largura; ++x) {
                int cx = x / CHUNK_SIZE;
                int cy = y / CHUNK_SIZE;
                if (m_chunk_sleep_enabled && !m_chunks[cy * m_chunks_x + cx].ativo) continue;
                atualizar_celula(x, y);
            }
        } else {
            for (int x = m_largura - 1; x >= 0; --x) {
                int cx = x / CHUNK_SIZE;
                int cy = y / CHUNK_SIZE;
                if (m_chunk_sleep_enabled && !m_chunks[cy * m_chunks_x + cx].ativo) continue;
                atualizar_celula(x, y);
            }
        }
    }

    for (auto& chunk : m_chunks) {
        if (chunk.next_dirty_rect.is_empty()) {
            chunk.ticks_parado++;
            if (chunk.ticks_parado >= TICKS_PARA_DORMIR) chunk.ativo = false;
        } else {
            chunk.ticks_parado = 0;
            chunk.ativo = true;
        }
        chunk.dirty_rect = chunk.next_dirty_rect;
        chunk.next_dirty_rect.reset();
        if (chunk.ativo) m_chunks_ativos_count++;
        std::fill(chunk.flags.begin(), chunk.flags.end(), 0);
    }
}

void SimCore::atualizar_flat_buffers() const {
    const size_t n = static_cast<size_t>(m_largura) * static_cast<size_t>(m_altura);
    if (m_flat_mat_id.size() != n) m_flat_mat_id.resize(n);
    if (m_flat_temp_vida.size() != n) m_flat_temp_vida.resize(n);
    if (m_flat_flags.size() != n) m_flat_flags.resize(n);

    for (int cy = 0; cy < m_chunks_y; ++cy) {
        for (int cx = 0; cx < m_chunks_x; ++cx) {
            const auto& chunk = m_chunks[cy * m_chunks_x + cx];
            for (int ly = 0; ly < CHUNK_SIZE; ++ly) {
                for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
                    int x = cx * CHUNK_SIZE + lx;
                    int y = cy * CHUNK_SIZE + ly;
                    if (x >= m_largura || y >= m_altura) continue;
                    size_t idx_c = static_cast<size_t>(ly) * CHUNK_SIZE + lx;
                    size_t idx_f = static_cast<size_t>(y) * m_largura + x;
                    m_flat_mat_id[idx_f] = chunk.mat_id[idx_c];
                    m_flat_temp_vida[idx_f] = chunk.temp_vida[idx_c];
                    m_flat_flags[idx_f] = chunk.flags[idx_c];
                }
            }
        }
    }
}

const uint16_t* SimCore::get_mat_id_data() const {
    atualizar_flat_buffers();
    return m_flat_mat_id.data();
}

const uint8_t* SimCore::get_temp_vida_data() const {
    atualizar_flat_buffers();
    return m_flat_temp_vida.data();
}

const uint8_t* SimCore::get_flags_data() const {
    atualizar_flat_buffers();
    return m_flat_flags.data();
}

uint64_t SimCore::hash_fnv1a64() const {
    const uint64_t basis = 1469598103934665603ULL;
    const uint64_t prime = 1099511628211ULL;
    uint64_t h = basis;
    auto mix = [&](uint8_t b) { h ^= b; h *= prime; };

    for (int y = 0; y < m_altura; ++y) {
        for (int x = 0; x < m_largura; ++x) {
            const auto& chunk = m_chunks[(y / CHUNK_SIZE) * m_chunks_x + (x / CHUNK_SIZE)];
            const uint16_t v = chunk.mat_id[(y % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE)];
            mix(static_cast<uint8_t>(v & 0xFF));
            mix(static_cast<uint8_t>((v >> 8) & 0xFF));
        }
    }
    for (int y = 0; y < m_altura; ++y) {
        for (int x = 0; x < m_largura; ++x) {
            const auto& chunk = m_chunks[(y / CHUNK_SIZE) * m_chunks_x + (x / CHUNK_SIZE)];
            mix(chunk.temp_vida[(y % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE)]);
        }
    }
    for (int y = 0; y < m_altura; ++y) {
        for (int x = 0; x < m_largura; ++x) {
            const auto& chunk = m_chunks[(y / CHUNK_SIZE) * m_chunks_x + (x / CHUNK_SIZE)];
            mix(chunk.flags[(y % CHUNK_SIZE) * CHUNK_SIZE + (x % CHUNK_SIZE)]);
        }
    }
    return h;
}

uint64_t SimCore::executar_cenario_teste() {
    preencher_tudo(MAT_AR);
    m_tick = 0;
    m_direcao_esq_para_dir = true;

    preencher_regiao(0, m_altura - 8, m_largura, m_altura, MAT_STONE);
    preencher_regiao(m_largura / 8, m_altura / 3,
                     3 * m_largura / 8, m_altura - 8, MAT_WATER);
    preencher_regiao(5 * m_largura / 8, m_altura / 4,
                     7 * m_largura / 8, m_altura - 8, MAT_SAND);

    for (int i = 0; i < 600; ++i) step();
    return hash_fnv1a64();
}

uint64_t SimCore::executar_cenario_teste_2() {
    preencher_tudo(MAT_AR);
    m_tick = 0;
    m_direcao_esq_para_dir = true;

    preencher_regiao(0, m_altura - 6, m_largura, m_altura, MAT_STONE);
    preencher_regiao(m_largura / 4, m_altura / 3,
                     3 * m_largura / 4, m_altura - 6, MAT_WATER);
    preencher_regiao(m_largura / 6, 2,
                     5 * m_largura / 6, m_altura / 3, MAT_SAND);

    for (int i = 0; i < 600; ++i) step();
    return hash_fnv1a64();
}

void SimCore::preencher_cenario_benchmark() {
    preencher_tudo(MAT_AR);
    m_tick = 0;
    m_direcao_esq_para_dir = true;
    preencher_regiao(0, 3 * m_altura / 4, m_largura, m_altura, MAT_STONE);
    preencher_regiao(0, 0, m_largura, m_altura / 4, MAT_WATER);
    preencher_regiao(0, m_altura / 4, m_largura, m_altura / 2, MAT_SAND);
}
