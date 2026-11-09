#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>

// Núcleo da simulação em chunks de 64x64 (Fase 2).
class SimCore {
public:
    SimCore(int largura, int altura, uint32_t seed);
    ~SimCore() = default;
    SimCore(const SimCore&) = delete;
    SimCore& operator=(const SimCore&) = delete;

    int get_largura() const { return m_largura; }
    int get_altura()  const { return m_altura; }

    void paint(int x, int y, int raio, uint16_t mat_id);
    void preencher_regiao(int x0, int y0, int x1, int y1, uint16_t mat_id);
    void preencher_tudo(uint16_t mat_id);

    void step();

    const uint16_t* get_mat_id_data()    const;
    uint16_t get_mat_id(int x, int y) const;


    const uint8_t*  get_temp_vida_data() const;
    const uint8_t*  get_flags_data()     const;

    // FNV-1a 64-bit sobre mat_id + temp_vida + flags.
    uint64_t hash_fnv1a64() const;

    // Cenário fixo para o teste de determinismo. Reinicia o estado.
    uint64_t executar_cenario_teste();

    // Cenário fixo para o teste de determinismo 2 (lago de água sobre areia com pedra).
    uint64_t executar_cenario_teste_2();

    // Cenário fixo para o benchmark. Reinicia o estado.
    void preencher_cenario_benchmark();

    // Debug: liga/desliga o sono dos chunks.
    void set_chunk_sleep_enabled(bool enabled) { m_chunk_sleep_enabled = enabled; }
    bool is_chunk_sleep_enabled() const { return m_chunk_sleep_enabled; }

    // Estatísticas de chunks.
    int get_chunks_ativos_count() const { return m_chunks_ativos_count; }
    int get_chunks_totais_count() const { return static_cast<int>(m_chunks.size()); }

private:
    static constexpr uint8_t FLAG_MOVIDA = 0x01;
    static constexpr int CHUNK_SIZE = 64;
    static constexpr int TICKS_PARA_DORMIR = 8;
    static constexpr int MUNDO_LARGURA = 2048;
    static constexpr int MUNDO_ALTURA = 1152;

    struct Rect {
        int x0, y0, x1, y1;
        void reset() { x0 = CHUNK_SIZE; y0 = CHUNK_SIZE; x1 = -1; y1 = -1; }
        void add(int x, int y) {
            x0 = std::min(x0, x); y0 = std::min(y0, y);
            x1 = std::max(x1, x); y1 = std::max(y1, y);
        }
        bool is_empty() const { return x1 < x0 || y1 < y0; }
    };

    struct Chunk {
        alignas(64) std::vector<uint16_t> mat_id;
        alignas(64) std::vector<uint8_t>  temp_vida;
        alignas(64) std::vector<uint8_t>  flags;

        Rect dirty_rect;
        Rect next_dirty_rect;
        int  ticks_parado = 0;
        bool ativo = true;

        Chunk() {
            mat_id.resize(CHUNK_SIZE * CHUNK_SIZE, 0);
            temp_vida.resize(CHUNK_SIZE * CHUNK_SIZE, 0);
            flags.resize(CHUNK_SIZE * CHUNK_SIZE, 0);
            dirty_rect.reset();
            next_dirty_rect.reset();
        }
    };

    int m_largura;
    int m_altura;
    int m_chunks_x;
    int m_chunks_y;

    std::vector<Chunk> m_chunks;

    uint32_t m_tick = 0;
    uint32_t m_seed_base;
    bool     m_direcao_esq_para_dir = true;
    bool     m_chunk_sleep_enabled = true;
    int      m_chunks_ativos_count = 0;

    mutable std::vector<uint16_t> m_flat_mat_id;
    mutable std::vector<uint8_t>  m_flat_temp_vida;
    mutable std::vector<uint8_t>  m_flat_flags;

    void atualizar_flat_buffers() const;
    void set_cell(int x, int y, uint16_t mat_id, uint8_t temp_vida = 0);
    void acordar_chunk(int cx, int cy);
    void marcar_dirty(int x, int y);

    // RNG sem estado derivado de seed + tick + x + y (função hash rápida).
    uint32_t rng_celula(int x, int y) const;

    void atualizar_celula(int x, int y);
    void atualizar_powder(int x, int y);
    void atualizar_liquid(int x, int y);
    bool tentar_mover(int x, int y, int dx, int dy);
    bool pode_trocar(uint16_t origem, uint16_t destino) const;

    bool dentro(int x, int y) const {
        return x >= 0 && x < m_largura && y >= 0 && y < m_altura;
    }
};
