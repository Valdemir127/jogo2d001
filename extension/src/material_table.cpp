#include "material_table.h"

using namespace godot;

MaterialTable::MaterialTable() {
    // Ordem dos IDs fixada: ar, pedra, areia, água.
    m_tabela[MAT_AR]    = { MAT_AR,    "Ar",    Estado::Ar,     Comportamento::Static,    0,    Color(0, 0, 0) };
    m_tabela[MAT_STONE] = { MAT_STONE, "Pedra", Estado::Solido, Comportamento::Static, 2500, Color("#808080") };
    m_tabela[MAT_SAND]  = { MAT_SAND,  "Areia", Estado::Powder, Comportamento::Powder, 1500, Color("#c2b280") };
    m_tabela[MAT_WATER] = { MAT_WATER, "Água",  Estado::Liquid, Comportamento::Liquid, 1000, Color("#3a7bd5") };
    m_total = 4;
}

const MaterialTable& MaterialTable::instancia() {
    static MaterialTable t;
    return t;
}