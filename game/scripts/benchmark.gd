extends SceneTree

const TAM_LARGURA := 512
const TAM_ALTURA  := 288
const TICKS       := 600

func _initialize() -> void:
    var sim := SimGrid.new()
    sim.inicializar(TAM_LARGURA, TAM_ALTURA, 0xDEADBEEF)
    sim.preencher_cenario_benchmark()
    sim.resetar_estatisticas()

    var t0 := Time.get_ticks_usec()
    for i in TICKS:
        sim.step()
    var t1 := Time.get_ticks_usec()

    var total_ms: float = float(t1 - t0) / 1000.0
    print("[benchmark] grid=%dx%d ticks=%d" % [TAM_LARGURA, TAM_ALTURA, TICKS])
    print("[benchmark] total=%.2f ms  medio=%.3f ms  max=%.3f ms" % [
        total_ms, sim.get_step_ms_medio(), sim.get_step_ms_maximo()
    ])
    print("[benchmark] hash final=%s" % sim.hash_atual())
    quit()