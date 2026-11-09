extends Node2D

const TAM_LARGURA    := 512
const TAM_ALTURA     := 288
const USAR_R16       := true      # trocar para false se FORMAT_R16 falhar no Forward+
const INTERVALO_INFO := 0.25

var sim: SimGrid
var tex_ids: ImageTexture
var tex_paleta: ImageTexture
var tex_rect: TextureRect
var label_info: Label
var shader_mat: ShaderMaterial

var material_atual: int = 2       # areia por padrão
var raio_pincel: int = 8
var _acum_info: float = 0.0
var _pintando: bool = false

func _ready() -> void:
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	sim = SimGrid.new()
	sim.inicializar(TAM_LARGURA, TAM_ALTURA, 0xDEADBEEF)

	tex_rect = TextureRect.new()
	tex_rect.position = Vector2.ZERO
	tex_rect.stretch_mode = TextureRect.STRETCH_SCALE
	tex_rect.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
	tex_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE   # <- deixa o clique passar
	shader_mat = ShaderMaterial.new()
	shader_mat.shader = load("res://game/shaders/paleta.gdshader")
	tex_rect.material = shader_mat
	add_child(tex_rect)

	_criar_paleta()
	_criar_ou_atualizar_textura_ids()
	tex_rect.texture = tex_ids                            # <- atribui a textura de verdade

	shader_mat.set_shader_parameter("tex_paleta", tex_paleta)
	shader_mat.set_shader_parameter("tam_grid", Vector2(TAM_LARGURA, TAM_ALTURA))
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
	var escala_x: float = vs.x / TAM_LARGURA
	var escala_y: float = vs.y / TAM_ALTURA
	var s: float = floor(min(escala_x, escala_y))
	if s < 1.0:
		s = 1.0
	tex_rect.scale = Vector2(s, s)

func _criar_paleta() -> void:
	# Paleta 1D: N texels = total de materiais; cor lida da MaterialTable.
	var total: int = sim.get_total_materiais()
	var img := Image.create_empty(total, 1, false, Image.FORMAT_RGBA8)
	for i in total:
		var cor: Color = sim.get_cor_material(i)
		img.set_pixel(i, 0, cor)
	tex_paleta = ImageTexture.create_from_image(img)

func _criar_ou_atualizar_textura_ids() -> void:
	if USAR_R16:
		var bytes: PackedByteArray = sim.get_ids()
		var img := Image.create_from_data(TAM_LARGURA, TAM_ALTURA, false,
										  Image.FORMAT_R16, bytes)
		if tex_ids == null:
			tex_ids = ImageTexture.create_from_image(img)
		else:
			tex_ids.update(img)
	else:
		var bytes: PackedByteArray = sim.get_ids_r8()
		var img := Image.create_from_data(TAM_LARGURA, TAM_ALTURA, false,
										  Image.FORMAT_R8, bytes)
		if tex_ids == null:
			tex_ids = ImageTexture.create_from_image(img)
		else:
			tex_ids.update(img)

func _process(delta: float) -> void:
	sim.step()
	_criar_ou_atualizar_textura_ids()

	if _pintando:
		var mp := get_local_mouse_position() / tex_rect.scale
		if mp.x >= 0 and mp.y >= 0 and mp.x < TAM_LARGURA and mp.y < TAM_ALTURA:
			sim.paint(int(mp.x), int(mp.y), raio_pincel, material_atual)
			_criar_ou_atualizar_textura_ids()

	_acum_info += delta
	if _acum_info >= INTERVALO_INFO:
		_acum_info = 0.0
		_atualizar_info()

func _atualizar_info() -> void:
	var fps := Engine.get_frames_per_second()
	var ms := sim.get_step_ms_medio()
	var mmax := sim.get_step_ms_maximo()
	var nome := sim.get_nome_material(material_atual)
	label_info.text = "FPS: %d | step médio: %.2f ms | máx: %.2f ms | mat: %s | raio: %d" % [
		fps, ms, mmax, nome, raio_pincel
	]

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		_pintando = event.pressed

	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_1: material_atual = 1   # pedra
			KEY_2: material_atual = 2   # areia
			KEY_3: material_atual = 3   # água
			KEY_0: material_atual = 0   # apagar
			KEY_EQUAL, KEY_PLUS: raio_pincel = min(raio_pincel + 2, 64)
			KEY_MINUS:           raio_pincel = max(raio_pincel - 2, 1)
