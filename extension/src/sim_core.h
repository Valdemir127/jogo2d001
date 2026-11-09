#pragma once

#include <cstdint>
#include <vector>

// Núcleo da simulação de grid (Fase 1: sem chunks, single-thread).
// Estado em SoA; nenhuma alocação dinâmica ocorre em step().
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

    const uint16_t* get_mat_id_data()    const { return m_mat_id.data(); }
    const uint8_t*  get_temp_vida_data() const { return m_temp_vida.data(); }
    const uint8_t*  get_flags_data()     const { return m_flags.data(); }

    // FNV-1a 64-bit sobre mat_id + temp_vida + flags.
    uint64_t hash_fnv1a64() const;

    // Cenário fixo para o teste de determinismo. Reinicia o estado.
    uint64_t executar_cenario_teste();

    // Cenário fixo para o benchmark. Reinicia o estado.
    void preencher_cenario_benchmark();

private:
    static constexpr uint8_t FLAG_MOVIDA = 0x01;

    int m_largura;
    int m_altura;

    std::vector<uint16_t> m_mat_id;
    std::vector<uint8_t>  m_temp_vida;
    std::vector<uint8_t>  m_flags;

    uint32_t m_tick = 0;
    uint32_t m_seed_base;
    bool     m_direcao_esq_para_dir = true;

    // RNG xorshift one-shot por célula (seed derivada de tick + x + y).
    uint32_t rng_celula(int x, int y) const;

    void atualizar_celula(int x, int y);
    void atualizar_powder(int x, int y);
    void atualizar_liquid(int x, int y);
    bool tentar_mover(int x, int y, int dx, int dy);
    bool pode_trocar(uint16_t origem, uint16_t destino) const;

    int  idx(int x, int y) const { return y * m_largura + x; }
    bool dentro(int x, int y) const {
        return x >= 0 && x < m_largura && y >= 0 && y < m_altura;
    }
};