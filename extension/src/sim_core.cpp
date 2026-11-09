#include "sim_core.h"
#include "material_table.h"

#include <algorithm>
#include <cstring>

SimCore::SimCore(int largura, int altura, uint32_t seed)
    : m_largura(largura), m_altura(altura), m_seed_base(seed) {
    const size_t n = static_cast<size_t>(largura) * static_cast<size_t>(altura);
    m_mat_id.resize(n, MAT_AR);
    m_temp_vida.resize(n, 0);
    m_flags.resize(n, 0);
}

void SimCore::paint(int cx, int cy, int raio, uint16_t mat_id) {
    if (mat_id >= MaterialTable::instancia().total()) return;
    const int r2 = raio * raio;
    for (int y = cy - raio; y <= cy + raio; ++y) {
        if (y < 0 || y >= m_altura) continue;
        for (int x = cx - raio; x <= cx + raio; ++x) {
            if (x < 0 || x >= m_largura) continue;
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy <= r2) {
                const int i = idx(x, y);
                m_mat_id[i] = mat_id;
                m_temp_vida[i] = 0;
            }
        }
    }
}

void SimCore::preencher_regiao(int x0, int y0, int x1, int y1, uint16_t mat_id) {
    for (int y = std::max(0, y0); y < std::min(m_altura, y1); ++y) {
        for (int x = std::max(0, x0); x < std::min(m_largura, x1); ++x) {
            const int i = idx(x, y);
            m_mat_id[i] = mat_id;
            m_temp_vida[i] = 0;
        }
    }
}

void SimCore::preencher_tudo(uint16_t mat_id) {
    std::fill(m_mat_id.begin(), m_mat_id.end(), mat_id);
    std::fill(m_temp_vida.begin(), m_temp_vida.end(), 0);
    std::fill(m_flags.begin(), m_flags.end(), 0);
}

uint32_t SimCore::rng_celula(int x, int y) const {
    // xorshift one-shot com seed determinística derivada de tick + posição.
    uint32_t s = m_seed_base
               ^ (m_tick * 0x9E3779B1u)
               ^ (static_cast<uint32_t>(x) * 0x85EBCA77u)
               ^ (static_cast<uint32_t>(y) * 0xC2B2AE3Du);
    if (s == 0) s = 1;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
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

    const int i = idx(x, y);
    const int j = idx(nx, ny);

    if (m_flags[j] & FLAG_MOVIDA) return false;

    const uint16_t origem  = m_mat_id[i];
    const uint16_t destino = m_mat_id[j];
    if (!pode_trocar(origem, destino)) return false;

    std::swap(m_mat_id[i], m_mat_id[j]);
    std::swap(m_temp_vida[i], m_temp_vida[j]);
    m_flags[i] |= FLAG_MOVIDA;
    m_flags[j] |= FLAG_MOVIDA;
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
    const int i = idx(x, y);
    if (m_flags[i] & FLAG_MOVIDA) return;

    const uint16_t mat = m_mat_id[i];
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

    // Varredura de baixo para cima (empilhamento correto);
    // direção horizontal alterna a cada tick para evitar viés.
    for (int y = m_altura - 1; y >= 0; --y) {
        if (m_direcao_esq_para_dir) {
            for (int x = 0; x < m_largura; ++x) atualizar_celula(x, y);
        } else {
            for (int x = m_largura - 1; x >= 0; --x) atualizar_celula(x, y);
        }
    }

    // Limpa flags para o próximo tick (mantém o hash limpo).
    std::fill(m_flags.begin(), m_flags.end(), 0);
}

uint64_t SimCore::hash_fnv1a64() const {
    const uint64_t basis = 1469598103934665603ULL;
    const uint64_t prime = 1099511628211ULL;
    uint64_t h = basis;
    auto mix = [&](uint8_t b) { h ^= b; h *= prime; };

    const size_t n = m_mat_id.size();
    for (size_t i = 0; i < n; ++i) {
        const uint16_t v = m_mat_id[i];
        mix(static_cast<uint8_t>(v & 0xFF));
        mix(static_cast<uint8_t>((v >> 8) & 0xFF));
    }
    for (size_t i = 0; i < n; ++i) mix(m_temp_vida[i]);
    for (size_t i = 0; i < n; ++i) mix(m_flags[i]);
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

void SimCore::preencher_cenario_benchmark() {
    preencher_tudo(MAT_AR);
    m_tick = 0;
    m_direcao_esq_para_dir = true;
    preencher_regiao(0, 3 * m_altura / 4, m_largura, m_altura, MAT_STONE);
    preencher_regiao(0, 0, m_largura, m_altura / 4, MAT_WATER);
    preencher_regiao(0, m_altura / 4, m_largura, m_altura / 2, MAT_SAND);
}