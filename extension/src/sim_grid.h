#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/color.hpp>

#include <memory>

class SimCore;

// Binding GDExtension: superfície pública pequena e estável.
class SimGrid : public godot::RefCounted {
    GDCLASS(SimGrid, godot::RefCounted)

public:
    SimGrid();
    ~SimGrid() override;

    void inicializar(int largura, int altura, uint32_t seed = 0xDEADBEEF);

    void step();
    void paint(int x, int y, int raio, int material_id);

    godot::PackedByteArray get_ids()    const;   // uint16 LE (2 bytes/célula)
    godot::PackedByteArray get_ids_r8() const;   // fallback R8 (1 byte/célula)

    int get_largura() const;
    int get_altura()  const;

    int         get_total_materiais() const;
    godot::String get_nome_material(int id) const;
    godot::Color  get_cor_material(int id)  const;

    // Determinismo: reinicia o estado e roda o cenário fixo. Retorna hash hex.
    godot::String executar_cenario_teste();

    // Benchmark: reinicia o estado com o cenário fixo.
    void preencher_cenario_benchmark();
    void resetar_estatisticas();
    double get_step_ms_medio()   const;
    double get_step_ms_maximo()  const;

    godot::String hash_atual() const;

protected:
    static void _bind_methods();

private:
    std::unique_ptr<SimCore> m_core;

    // Estatísticas de profiling (só leitura fora do tick).
    double   m_step_ms_total = 0.0;
    double   m_step_ms_max   = 0.0;
    uint64_t m_step_count    = 0;

    // Buffer reutilizado para get_ids (evita realocação por frame).
    mutable godot::PackedByteArray m_buf_r16;
    mutable godot::PackedByteArray m_buf_r8;
};