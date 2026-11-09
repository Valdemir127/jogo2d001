#pragma once

#include <cstdint>
#include <vector>

// Núcleo da simulação em chunks de 64x64 (Fase 2 - Parte 1: armazenamento em chunks, SoA).
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
    const uint8_t*  get_temp_vida_data() const;
    const uint8_t*  get_flags_data()     const;

    // FNV-1a 64-bit sobre mat_id + temp_vida + flags.
    uint64_t hash_fnv1a64() const;

    // Cenário fixo para o teste de determinismo. Reinicia o estado.
    uint64_t executar_cenario_teste();

    // Cenário fixo para o benchmark. Reinicia o estado.
    void preencher_cenario_benchmark();

private:
    static constexpr uint8_t FLAG_MOVIDA = 0x01;
    static constexpr int CHUNK_SIZE = 64;

    struct Chunk {
        alignas(64) std::vector<uint16_t> mat_id;
        alignas(64) std::vector<uint8_t>  temp_vida;
        alignas(64) std::vector<uint8_t>  flags;

        Chunk() {
            mat_id.resize(CHUNK_SIZE * CHUNK_SIZE, 0);
            temp_vida.resize(CHUNK_SIZE * CHUNK_SIZE, 0);
            flags.resize(CHUNK_SIZE * CHUNK_SIZE, 0);
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

    mutable std::vector<uint16_t> m_flat_mat_id;
    mutable std::vector<uint8_t>  m_flat_temp_vida;
    mutable std::vector<uint8_t>  m_flat_flags;

    void atualizar_flat_buffers() const;
    void set_cell(int x, int y, uint16_t mat_id, uint8_t temp_vida = 0);

    // RNG xorshift one-shot por célula (seed derivada de tick + x + y).
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
