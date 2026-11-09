#include "sim_grid.h"
#include "sim_core.h"
#include "material_table.h"

#include <chrono>
#include <cstdio>

using namespace godot;

SimGrid::SimGrid() = default;
SimGrid::~SimGrid() = default;

void SimGrid::_bind_methods() {
    ClassDB::bind_method(D_METHOD("inicializar", "largura", "altura", "seed"),
                         &SimGrid::inicializar);
    ClassDB::bind_method(D_METHOD("step"), &SimGrid::step);
    ClassDB::bind_method(D_METHOD("paint", "x", "y", "raio", "material_id"),
                         &SimGrid::paint);
    ClassDB::bind_method(D_METHOD("get_ids"), &SimGrid::get_ids);
    ClassDB::bind_method(D_METHOD("get_ids_r8"), &SimGrid::get_ids_r8);
    ClassDB::bind_method(D_METHOD("get_largura"), &SimGrid::get_largura);
    ClassDB::bind_method(D_METHOD("get_altura"),  &SimGrid::get_altura);
    ClassDB::bind_method(D_METHOD("get_total_materiais"), &SimGrid::get_total_materiais);
    ClassDB::bind_method(D_METHOD("get_nome_material", "id"), &SimGrid::get_nome_material);
    ClassDB::bind_method(D_METHOD("get_cor_material",  "id"), &SimGrid::get_cor_material);
    ClassDB::bind_method(D_METHOD("executar_cenario_teste"), &SimGrid::executar_cenario_teste);
    ClassDB::bind_method(D_METHOD("preencher_cenario_benchmark"),
                         &SimGrid::preencher_cenario_benchmark);
    ClassDB::bind_method(D_METHOD("resetar_estatisticas"), &SimGrid::resetar_estatisticas);
    ClassDB::bind_method(D_METHOD("get_step_ms_medio"),  &SimGrid::get_step_ms_medio);
    ClassDB::bind_method(D_METHOD("get_step_ms_maximo"), &SimGrid::get_step_ms_maximo);
    ClassDB::bind_method(D_METHOD("hash_atual"), &SimGrid::hash_atual);
}

void SimGrid::inicializar(int largura, int altura, uint32_t seed) {
    m_core = std::make_unique<SimCore>(largura, altura, seed);
    resetar_estatisticas();
}

void SimGrid::step() {
    if (!m_core) return;
    using clock = std::chrono::high_resolution_clock;
    const auto t0 = clock::now();
    m_core->step();
    const auto t1 = clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    m_step_ms_total += ms;
    if (ms > m_step_ms_max) m_step_ms_max = ms;
    ++m_step_count;
}

void SimGrid::paint(int x, int y, int raio, int material_id) {
    if (!m_core) return;
    if (material_id < 0) material_id = 0;
    m_core->paint(x, y, raio, static_cast<uint16_t>(material_id));
}

PackedByteArray SimGrid::get_ids() const {
    if (!m_core) return PackedByteArray();
    const int w = m_core->get_largura();
    const int h = m_core->get_altura();
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
    m_buf_r16.resize(static_cast<int64_t>(n * 2));
    uint8_t* dst = m_buf_r16.ptrw();
    const uint16_t* src = m_core->get_mat_id_data();
    for (size_t i = 0; i < n; ++i) {
        dst[i * 2]     = static_cast<uint8_t>(src[i] & 0xFF);
        dst[i * 2 + 1] = static_cast<uint8_t>((src[i] >> 8) & 0xFF);
    }
    return m_buf_r16;
}

PackedByteArray SimGrid::get_ids_r8() const {
    if (!m_core) return PackedByteArray();
    const int w = m_core->get_largura();
    const int h = m_core->get_altura();
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
    m_buf_r8.resize(static_cast<int64_t>(n));
    uint8_t* dst = m_buf_r8.ptrw();
    const uint16_t* src = m_core->get_mat_id_data();
    for (size_t i = 0; i < n; ++i) {
        dst[i] = src[i] > 255 ? 255 : static_cast<uint8_t>(src[i]);
    }
    return m_buf_r8;
}

int SimGrid::get_largura() const { return m_core ? m_core->get_largura() : 0; }
int SimGrid::get_altura()  const { return m_core ? m_core->get_altura()  : 0; }

int SimGrid::get_total_materiais() const { return MaterialTable::instancia().total(); }

String SimGrid::get_nome_material(int id) const {
    const auto& t = MaterialTable::instancia();
    if (id < 0 || id >= t.total()) return String();
    return String(t.get(static_cast<uint16_t>(id)).nome);
}

Color SimGrid::get_cor_material(int id) const {
    const auto& t = MaterialTable::instancia();
    if (id < 0 || id >= t.total()) return Color(0, 0, 0);
    return t.get(static_cast<uint16_t>(id)).cor;
}

String SimGrid::executar_cenario_teste() {
    if (!m_core) return String();
    const uint64_t h = m_core->executar_cenario_teste();
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
    return String(buf);
}

void SimGrid::preencher_cenario_benchmark() {
    if (m_core) m_core->preencher_cenario_benchmark();
}

void SimGrid::resetar_estatisticas() {
    m_step_ms_total = 0.0;
    m_step_ms_max   = 0.0;
    m_step_count    = 0;
}

double SimGrid::get_step_ms_medio() const {
    return m_step_count > 0 ? m_step_ms_total / static_cast<double>(m_step_count) : 0.0;
}
double SimGrid::get_step_ms_maximo() const { return m_step_ms_max; }

String SimGrid::hash_atual() const {
    if (!m_core) return String();
    const uint64_t h = m_core->hash_fnv1a64();
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%016llx", static_cast<unsigned long long>(h));
    return String(buf);
}