#pragma once

#include <cstdint>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

// Estados e comportamentos disponíveis na Fase 1.
// Esta tabela será preenchida pelo registro de mods no futuro;
// por enquanto os valores são fixos no código.
enum class Estado : uint8_t { Ar, Solido, Powder, Liquid, Gas };
enum class Comportamento : uint8_t { Static, Powder, Liquid };

// IDs fixos da Fase 1.
constexpr uint16_t MAT_AR    = 0;
constexpr uint16_t MAT_STONE = 1;
constexpr uint16_t MAT_SAND  = 2;
constexpr uint16_t MAT_WATER = 3;

struct Material {
    uint16_t      id;
    const char*   nome;
    Estado        estado;
    Comportamento comportamento;
    uint16_t      densidade;   // água = 1000; ar = 0
    godot::Color  cor;
};

// Tabela indexada por ID, sem chamadas virtuais no loop de simulação.
class MaterialTable {
public:
    static const MaterialTable& instancia();

    int total() const { return m_total; }
    const Material& get(uint16_t id) const { return m_tabela[id]; }

private:
    MaterialTable();
    Material m_tabela[16];
    int m_total = 0;
};