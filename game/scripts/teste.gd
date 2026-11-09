extends Node2D

const USAR_R16       := true
const INTERVALO_INFO := 0.10
const TAM_VENTANA    := Vector2i(640, 360)

var sim: SimGrid
var tex_ids: ImageTexture
var tex_paleta: ImageTexture
var tex_rect: TextureRect
var label_info: Label
var shader_mat: ShaderMaterial

var camera_pos: Vector2i = Vector2i.ZERO
var zoom: int = 1

var material_atual: int = 2
var raio_pincel: int = 8
var _acum_info: float = 0.0
var _pintando: bool = false

func _ready() -> void:
	var args := OS.get_cmdline_args()
	var user_args := OS.get_cmdline_user_args()
	print("Args: ", args, " | User Args: ", user_args)

	if "--test-determinism" in args or "--test-determinism" in user_args:
		print(">>> INICIANDO TESTE DE DETERMINISMO <<<")
		var sim_teste := SimGrid.new()
		sim_teste.inicializar(512, 288, 0xDEADBEEF)
		var h := sim_teste.executar_cenario_teste()
		print("[det] hash=", h)
		get_tree().quit()
		return

	print(">>> INICIANDO JOGO NORMAL <<<")
	sim = SimGrid.new()
	sim.inicializar(2048, 1152, 0xDEADBEEF)

	tex_rect = TextureRect.new()
	tex_rect.position = Vector2.ZERO
	tex_rect.stretch_mode = TextureRect.STRETCH_SCALE
	tex_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	tex_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	shader_mat = ShaderMaterial.new()
	shader_mat.shader = load("res://game/shaders/paleta.gdshader")
	tex_rect.material = shader_mat
	add_child(tex_rect)

	_criar_paleta()
	_criar_ou_atualizar_textura_ids()
	tex_rect.texture = tex_ids

	shader_mat.set_shader_parameter("tex_paleta", tex_paleta)
	shader_mat.set_shader_parameter("tam_grid", TAM_VENTANA)
	shader_mat.set_shader_parameter("usa_r16", USAR_R16)

	label_info = Label.new()
	label_info.position = Vector2(10, 10)
	label_info.add_theme_color_override("font_color", Color.WHITE)
	label_info.add_theme_color_override("font_shadow_color", Color.BLACK)
	label_info.add_theme_constant_override("shadow_offset_x", 1)
	label_info.add_theme_constant_override("shadow_offset_y", 1)
	add_child(label_info)

	get_viewport().size_changed.connect(_ajustar_escala)
	_ajustar_escala()

func _ajustar_escala() -> void:
	var vs := get_viewport_rect().size
	var escala_x: float = vs.x / TAM_VENTANA.x
	var escala_y: float = vs.y / TAM_VENTANA.y
	var s: float = floor(min(escala_x, escala_y))
	if s < 1.0:
		s = 1.0
	tex_rect.scale = Vector2(s, s)

func _process(delta: float) -> void:
	sim.step()

	_criar_ou_atualizar_textura_ids()

	if _pintando:
		var mp := get_local_mouse_position() / tex_rect.scale
		var world_pos_x: float = camera_pos.x + mp.x * zoom
		var world_pos_y: float = camera_pos.y + mp.y * zoom
		if world_pos_x >= 0 and world_pos_y >= 0 and world_pos_x < sim.get_largura() and world_pos_y < sim.get_altura():
			sim.paint(int(world_pos_x), int(world_pos_y), raio_pincel * zoom, material_atual)
			_criar_ou_atualizar_textura_ids()

	_acum_info += delta
	if _acum_info >= INTERVALO_INFO:
		_acum_info = 0.0
		_atualizar_info()

func _criar_paleta() -> void:
	var total: int = sim.get_total_materiais()
	var img := Image.create_empty(total, 1, false, Image.FORMAT_RGBA8)
	for i in total:
		var cor: Color = sim.get_cor_material(i)
		img.set_pixel(i, 0, cor)
	tex_paleta = ImageTexture.create_from_image(img)

func _criar_ou_atualizar_textura_ids() -> void:
	var bytes: PackedByteArray
	if USAR_R16:
		bytes = sim.get_ids_camara(camera_pos.x, camera_pos.y, TAM_VENTANA.x, TAM_VENTANA.y, zoom)
		var img := Image.create_from_data(TAM_VENTANA.x, TAM_VENTANA.y, false,
										  Image.FORMAT_R16, bytes)
		if tex_ids == null:
			tex_ids = ImageTexture.create_from_image(img)
		else:
			tex_ids.update(img)
	else:
		bytes = sim.get_ids_camara(camera_pos.x, camera_pos.y, TAM_VENTANA.x, TAM_VENTANA.y, zoom)
		var img := Image.create_from_data(TAM_VENTANA.x, TAM_VENTANA.y, false,
										  Image.FORMAT_R8, bytes)
		if tex_ids == null:
			tex_ids = ImageTexture.create_from_image(img)
		else:
			tex_ids.update(img)

func _atualizar_info() -> void:
	var fps := Engine.get_frames_per_second()
	var ms := sim.get_step_ms_medio()
	var mmax := sim.get_step_ms_maximo()
	var nome := sim.get_nome_material(material_atual)
	var ativos := sim.get_chunks_ativos()
	var totais := sim.get_chunks_totais()
	label_info.text = "FPS: %d | step: %.2f (max %.2f) ms\nChunks Ativos: %d/%d\nCamera: %d,%d | Zoom: %dx\nMat: %s | Raio: %d" % [
		fps, ms, mmax, ativos, totais, camera_pos.x, camera_pos.y, zoom, nome, raio_pincel
	]

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		_pintando = event.pressed

	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_W, KEY_UP: camera_pos.y = max(0, camera_pos.y - 32 * zoom)
			KEY_S: camera_pos.y = min(sim.get_altura() - TAM_VENTANA.y * zoom, camera_pos.y + 32 * zoom)
			KEY_A, KEY_LEFT: camera_pos.x = max(0, camera_pos.x - 32 * zoom)
			KEY_D, KEY_RIGHT: camera_pos.x = min(sim.get_largura() - TAM_VENTANA.x * zoom, camera_pos.x + 32 * zoom)
			KEY_Q: zoom = max(1, zoom - 1)
			KEY_E: zoom = min(8, zoom + 1)
			KEY_1: material_atual = 1
			KEY_2: material_atual = 2
			KEY_3: material_atual = 3
			KEY_0: material_atual = 0
			KEY_EQUAL, KEY_PLUS: raio_pincel = min(raio_pincel + 2, 64)
			KEY_MINUS:           raio_pincel = max(raio_pincel - 2, 1)
