extends SceneTree
# Roda o cenário duas vezes no mesmo processo e compara com o golden.

const GOLDEN_PATH := "res://tests/determinism_golden.txt"

func _initialize() -> void:
	print("Iniciando teste de determinismo...")
	var h1 := _rodar(true)
	var h2 := _rodar(true)
	var h1_nosleep := _rodar(false)
	print("[det] hash1 (sleep)=%s" % h1)
	print("[det] hash2 (sleep)=%s" % h2)
	print("[det] hash  (nosleep)=%s" % h1_nosleep)

	if h1 != h2:
		printerr("[det] FALHA: hashes diferentes na mesma execucao")
		quit(1)
		return

	if h1 != h1_nosleep:
		printerr("[det] FALHA: resultado com sono difere do resultado sem sono! (sleep=%s, nosleep=%s)" % [h1, h1_nosleep])
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
	print("[det] OK (Equivalência de sono validada!)")
	quit(0)

func _rodar(sleep_enabled: bool) -> String:
	var sim := SimGrid.new()
	sim.inicializar(512, 288, 0xDEADBEEF)
	sim.set_chunk_sleep_enabled(sleep_enabled)
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
