MODULE := engines/sci

MODULE_OBJS := \
	console.o \
	debugger.o \
	decompressor.o \
	detection.o \
	event.o \
	resource/resource.o \
	resource/manager.o \
	resource/patcher.o \
	resource/source.o \
	resource/sources/directory.o \
	resource/sources/extmap.o \
	resource/sources/intmap.o \
	resource/sources/extaudiomap.o \
	resource/sources/macresfork.o \
	resource/sources/wave.o \
	resource/sources/audiovolume.o \
	resource/sources/volume.o \
	resource/sources/patch.o \
	sci.o \
	time.o \
	util.o \
	engine/features.o \
	engine/file.o \
	engine/gc.o \
	engine/guest_additions.o \
	engine/kernel.o \
	engine/kevent.o \
	engine/kfile.o \
	engine/kgraphics.o \
	engine/klists.o \
	engine/kmath.o \
	engine/kmenu.o \
	engine/kmisc.o \
	engine/kmovement.o \
	engine/kparse.o \
	engine/kpathing.o \
	engine/kscripts.o \
	engine/ksound.o \
	engine/kstring.o \
	engine/kvideo.o \
	engine/message.o \
	engine/object.o \
	engine/savegame.o \
	engine/script.o \
	engine/scriptdebug.o \
	engine/script_patches.o \
	engine/selector.o \
	engine/seg_manager.o \
	engine/segment.o \
	engine/state.o \
	engine/static_selectors.o \
	engine/vm.o \
	engine/vm_types.o \
	engine/workarounds.o \
	graphics/animate.o \
	graphics/cache.o \
	graphics/compare.o \
	graphics/controls16.o \
	graphics/coordadjuster.o \
	graphics/cursor.o \
	graphics/font.o \
	graphics/fontsjis.o \
	graphics/maciconbar.o \
	graphics/menu.o \
	graphics/paint16.o \
	graphics/palette.o \
	graphics/picture.o \
	graphics/portrait.o \
	graphics/ports.o \
	graphics/remap.o \
	graphics/screen.o \
	graphics/text16.o \
	graphics/transitions.o \
	graphics/view.o \
	parser/grammar.o \
	parser/said.o \
	parser/vocabulary.o \
	sound/audio.o \
	sound/sound.o \
	sound/sci0sound.o \
	sound/sci1sound.o \
	sound/sync.o \
	sound/drivers/adlib.o \
	sound/drivers/amigamac.o \
	sound/drivers/genmidi.o \
	sound/drivers/mt32.o \
	video/seq_decoder.o


ifdef ENABLE_SCI32
MODULE_OBJS += \
	engine/kgraphics32.o \
	graphics/bitmap32.o \
	graphics/celobj32.o \
	graphics/controls32.o \
	graphics/frameout.o \
	graphics/paint32.o \
	graphics/plane32.o \
	graphics/palette32.o \
	graphics/remap32.o \
	graphics/screen_item32.o \
	graphics/text32.o \
	graphics/transitions32.o \
	graphics/video32.o \
	graphics/cursor32.o \
	resource/sources/chunk32.o \
	sound/audio32.o \
	sound/decoders/sol.o \
	video/robot_decoder.o

ifdef ENABLE_SCI32S2
MODULE_OBJS += \
	resource/sources/pe.o \
	resource/sources/solvolume.o \
	s2/bitmap.o \
	s2/bitmap_manager.o \
	s2/button.o \
	s2/control.o \
	s2/cursor.o \
	s2/debugger.o \
	s2/dialog.o \
	s2/engine.o \
	s2/exit.o \
	s2/game.o \
	s2/hotspot.o \
	s2/interface.o \
	s2/inventory.o \
	s2/inventory_manager.o \
	s2/inventory_object.o \
	s2/kernel.o \
	s2/message_box.o \
	s2/movie_manager.o \
	s2/panorama_image.o \
	s2/panorama_sprite.o \
	s2/phone_manager.o \
	s2/room.o \
	s2/room_manager.o \
	s2/savegame.o \
	s2/scoring_manager.o \
	s2/sound_manager.o \
	s2/rooms/1000.o \
	s2/rooms/6000.o \
	s2/rooms/10000.o \
	s2/rooms/global.o \
	s2/rooms/phone.o \
	s2/system/glbutton.o \
	s2/system/glcast.o \
	s2/system/glcel.o \
	s2/system/glcue.o \
	s2/system/glcursor.o \
	s2/system/glcycler.o \
	s2/system/glevent.o \
	s2/system/glevent_handler_set.o \
	s2/system/glfeature.o \
	s2/system/glmovie.o \
	s2/system/glmovie_player.o \
	s2/system/globject.o \
	s2/system/glpanorama.o \
	s2/system/glplane.o \
	s2/system/glplane_manager.o \
	s2/system/glpoly.o \
	s2/system/glquit_handler.o \
	s2/system/glrobot.o \
	s2/system/glscreen_item.o \
	s2/system/glscript.o \
	s2/system/glset.o \
	s2/system/glsound.o \
	s2/system/glsound_manager.o \
	s2/system/gltarget.o \
	s2/system/gltimer.o \
	s2/system/gluser.o \
	s2/system/glvr_plane.o
endif
endif

# This module can be built as a plugin
ifeq ($(ENABLE_SCI), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk
