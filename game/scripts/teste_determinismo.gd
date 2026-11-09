extends SceneTree
# Roda o cenário duas vezes no mesmo processo e compara com o golden.

const GOLDEN_PATH := "res://tests/determinism_golden.txt"

func _initialize() -> void:
    print("Iniciando teste de determinismo...")
    var h1 := _rodar()
    var h2 := _rodar()
    print("[det] hash1=%s" % h1)
    print("[det] hash2=%s" % h2)

    if h1 != h2:
        printerr("[det] FALHA: hashes diferentes na mesma execucao")
        quit(1)
        return

    var esperado := _ler_golden()
    if esperado == "":
        print("[det] golden vazio — grave com: %s" % h1)
        quit(0)
        return
    if h1 != esperado:
        printerr("[det] FALHA: esperado=%s obtido=%s" % [esperado, h1])
        quit(1)
        return
    print("[det] OK")
    quit(0)

func _rodar() -> String:
    var sim := SimGrid.new()
    sim.inicializar(512, 288, 0xDEADBEEF)
    return sim.executar_cenario_teste()

func _ler_golden() -> String:
    if not FileAccess.file_exists(GOLDEN_PATH):
        return ""
    var f := FileAccess.open(GOLDEN_PATH, FileAccess.READ)
    while not f.eof_reached():
        var l := f.get_line().strip_edges()
        if l != "" and not l.begins_with("#"):
            return l
    return ""