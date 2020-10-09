project('chaos', 'c')

cc = meson.get_compiler('c')

gtk = dependency('gtk+-3.0')
vte = dependency('vte-2.91')

executable(
	'chaos',
	files('main.c'),
	link_with: [],
	include_directories: ['.'],
	dependencies: [gtk, vte],
	install: true,
)