/* Renames of old pcb_ prefixed (and unprefixed) symbols to the rnd_ prefixed
   variants. Included from librnd/config.h when PCB_REGISTER_ACTIONS is
   #defined.

   This mechanism is used for the transition period before librnd is moved
   out to a new repository. Ringdove apps may use the old names so they
   work with multiple versions of librnd. Once the move-out takes place,
   a new epoch is defined and all ringdove apps must use the rnd_ prefixed
   symbols only (and this file will be removed).
*/
#define pcb_register_action rnd_register_action
#define pcb_register_actions rnd_register_actions
#define PCB_REGISTER_ACTIONS RND_REGISTER_ACTIONS
#define pcb_action_s rnd_action_s
#define pcb_action_t rnd_action_t
#define pcb_fgw rnd_fgw
#define PCB_PTR_DOMAIN_IDPATH RND_PTR_DOMAIN_IDPATH
#define PCB_PTR_DOMAIN_IDPATH_LIST RND_PTR_DOMAIN_IDPATH_LIST
#define pcb_actions_init rnd_actions_init
#define pcb_actions_uninit rnd_actions_uninit
#define pcb_print_actions rnd_print_actions
#define pcb_dump_actions rnd_dump_actions
#define pcb_find_action rnd_find_action
#define pcb_remove_actions rnd_remove_actions
#define pcb_remove_actions_by_cookie rnd_remove_actions_by_cookie
#define pcb_action rnd_action
#define pcb_actionva rnd_actionva
#define pcb_actionv rnd_actionv
#define pcb_actionv_ rnd_actionv_
#define pcb_actionl rnd_actionl
#define pcb_actionv_bin rnd_actionv_bin
#define pcb_parse_command rnd_parse_command
#define pcb_parse_actions rnd_parse_actions
#define pcb_cli_prompt rnd_cli_prompt
#define pcb_cli_enter rnd_cli_enter
#define pcb_cli_leave rnd_cli_leave
#define pcb_cli_tab rnd_cli_tab
#define pcb_cli_edit rnd_cli_edit
#define pcb_cli_mouse rnd_cli_mouse
#define pcb_cli_uninit rnd_cli_uninit
#define pcb_hid_get_coords rnd_hid_get_coords
#define PCB_ACTION_MAX_ARGS RND_ACTION_MAX_ARGS
#define PCB_ACT_HIDLIB RND_ACT_HIDLIB
#define pcb_act_lookup rnd_act_lookup
#define pcb_make_action_name rnd_make_action_name
#define pcb_aname rnd_aname
#define pcb_act_result rnd_act_result
#define PCB_ACT_CALL_C RND_ACT_CALL_C
#define PCB_ACT_CONVARG RND_PCB_ACT_CONVARG
#define PCB_ACT_IRES RND_ACT_IRES
#define PCB_ACT_DRES RND_ACT_DRES
#define PCB_ACT_FAIL RND_ACT_FAIL
#define pcb_fgw_types_e rnd_fgw_types_e
#define pcb_hidlib_t rnd_hidlib_t
#define pcb_coord_t rnd_coord_t
#define pcb_bool rnd_bool
#define pcb_message rnd_message
#define PCB_ACTION_NAME_MAX RND_ACTION_NAME_MAX
#define pcb_attribute_list_s rnd_attribute_list_s
#define pcb_attribute_list_t rnd_attribute_list_t
#define pcb_attribute_t rnd_attribute_t
#define pcb_attribute_s rnd_attribute_s
#define pcb_attribute_get rnd_attribute_get
#define pcb_attribute_get_ptr rnd_attribute_get_ptr
#define pcb_attribute_get_namespace rnd_attribute_get_namespace
#define pcb_attribute_get_namespace_ptr rnd_attribute_get_namespace_ptr
#define pcb_attribute_put rnd_attribute_put
#define pcb_attrib_get rnd_attrib_get
#define pcb_attrib_put rnd_attrib_put
#define pcb_attribute_remove rnd_attribute_remove
#define pcb_attrib_remove rnd_attrib_remove
#define pcb_attribute_remove_idx rnd_attribute_remove_idx
#define pcb_attribute_free rnd_attribute_free
#define pcb_attribute_copy_all rnd_attribute_copy_all
#define pcb_attribute_copyback_begin rnd_attribute_copyback_begin
#define pcb_attribute_copyback rnd_attribute_copyback
#define pcb_attribute_copyback_end rnd_attribute_copyback_end
#define pcb_attrib_compat_set_intconn rnd_attrib_compat_set_intconn
#define base64_write_right rnd_base64_write_right
#define base64_parse_grow rnd_base64_parse_grow
#define pcb_box_list_s rnd_box_list_s
#define pcb_box_list_t rnd_box_list_t
#define PCB_BOX_ROTATE_CW RND_BOX_ROTATE_CW
#define PCB_BOX_ROTATE_TO_NORTH RND_BOX_ROTATE_TO_NORTH
#define PCB_BOX_ROTATE_FROM_NORTH RND_BOX_ROTATE_FROM_NORTH
#define PCB_BOX_CENTER_X RND_BOX_CENTER_X
#define PCB_BOX_CENTER_Y RND_BOX_CENTER_Y
#define PCB_MOVE_POINT RND_MOVE_POINT
#define PCB_BOX_MOVE_LOWLEVEL RND_BOX_MOVE_LOWLEVEL
#define pcb_point_in_box rnd_point_in_box
#define pcb_point_in_closed_box rnd_point_in_closed_box
#define pcb_box_is_good rnd_box_is_good
#define pcb_box_intersect rnd_box_intersect
#define pcb_closest_pcb_point_in_box rnd_closest_cheap_point_in_box
#define pcb_box_in_box rnd_box_in_box
#define pcb_clip_box rnd_clip_box
#define pcb_shrink_box rnd_shrink_box
#define pcb_bloat_box rnd_bloat_box
#define pcb_box_center rnd_box_center
#define pcb_box_corner rnd_box_corner
#define pcb_point_box rnd_point_box
#define pcb_close_box rnd_close_box
#define pcb_dist2_to_box rnd_dist2_to_box
#define pcb_box_bump_box rnd_box_bump_box
#define pcb_box_bump_point rnd_box_bump_point
#define pcb_box_rotate90 rnd_box_rotate90
#define pcb_box_enlarge rnd_box_enlarge
#define PCB_NORTH RND_NORTH
#define PCB_EAST RND_EAST
#define PCB_SOUTH RND_SOUTH
#define PCB_WEST RND_WEST
#define PCB_NE RND_NE
#define PCB_SE RND_SE
#define PCB_SW RND_SW
#define PCB_NW RND_NW
#define PCB_ANY_DIR RND_ANY_DIR
#define pcb_box_t rnd_box_t
#define pcb_direction_t rnd_direction_t
#define pcb_cheap_point_s rnd_cheap_point_s
#define pcb_cheap_point_t rnd_cheap_point_t
#define pcb_distance rnd_distance
#define pcb_cardinal_t rnd_cardinal_t
#define PCB_INLINE RND_INLINE
#define PCB_HAVE_SETENV RND_HAVE_SETENV
#define PCB_HAVE_PUTENV RND_HAVE_PUTENV
#define PCB_HAVE_USLEEP RND_HAVE_USLEEP
#define PCB_HAVE_WSLEEP RND_HAVE_WSLEEP
#define PCB_HAVE_SELECT RND_HAVE_SELECT
#define HAVE_SNPRINTF RND_HAVE_SNPRINTF
#define HAVE_VSNPRINTF RND_HAVE_VSNPRINTF
#define HAVE_GETCWD RND_HAVE_GETCWD
#define HAVE__GETCWD RND_HAVE__GETCWD
#define HAVE_GETWD RND_HAVE_GETWD
#define HAVE_GD_H RND_HAVE_GD_H
#define HAVE_GDIMAGEGIF RND_HAVE_GDIMAGEGIF
#define HAVE_GDIMAGEJPEG RND_HAVE_GDIMAGEJPEG
#define HAVE_GDIMAGEPNG RND_HAVE_GDIMAGEPNG
#define HAVE_GD_RESOLUTION RND_HAVE_GD_RESOLUTION
#define HAVE_GETPWUID RND_HAVE_GETPWUID
#define HAVE_RINT RND_HAVE_RINT
#define HAVE_ROUND RND_HAVE_ROUND
#define WRAP_S_ISLNK RND_WRAP_S_ISLNK
#define HAVE_XINERAMA RND_HAVE_XINERAMA
#define HAVE_XRENDER RND_HAVE_XRENDER
#define USE_LOADLIBRARY RND_USE_LOADLIBRARY
#define HAVE_MKDTEMP RND_HAVE_MKDTEMP
#define HAVE_REALPATH RND_HAVE_REALPATH
#define USE_FORK_WAIT RND_USE_FORK_WAIT
#define USE_SPAWNVP RND_USE_SPAWNVP
#define USE_MKDIR RND_USE_MKDIR
#define USE__MKDIR RND_USE__MKDIR
#define MKDIR_NUM_ARGS RND_MKDIR_NUM_ARGS
#define PCB_HAVE_SIGPIPE RND_HAVE_SIGPIPE
#define PCB_HAVE_SIGSEGV RND_HAVE_SIGSEGV
#define PCB_HAVE_SIGABRT RND_HAVE_SIGABRT
#define PCB_HAVE_SIGINT RND_HAVE_SIGINT
#define PCB_HAVE_SIGHUP RND_HAVE_SIGHUP
#define PCB_HAVE_SIGTERM RND_HAVE_SIGTERM
#define PCB_HAVE_SIGQUIT RND_HAVE_SIGQUIT
#define PCB_HAVE_SYS_FUNGW RND_HAVE_SYS_FUNGW
#define PCB_DIR_SEPARATOR_C RND_DIR_SEPARATOR_C
#define PCB_DIR_SEPARATOR_S RND_DIR_SEPARATOR_S
#define PCB_PATH_DELIMETER RND_PATH_DELIMETER
#define HAS_ATEXIT RND_HAS_ATEXIT
#define HAVE_UNISTD_H RND_HAVE_UNISTD_H
#define COORD_MAX RND_COORD_MAX
#define PCB_FUNC_UNUSED RND_FUNC_UNUSED
#define pcb_color_s rnd_color_s
#define pcb_color_t rnd_color_t
#define pcb_color_black rnd_color_black
#define pcb_color_white rnd_color_white
#define pcb_color_cyan rnd_color_cyan
#define pcb_color_red rnd_color_red
#define pcb_color_blue rnd_color_blue
#define pcb_color_grey33 rnd_color_grey33
#define pcb_color_magenta rnd_color_magenta
#define pcb_color_golden rnd_color_golden
#define pcb_color_drill rnd_color_drill
#define pcb_color_load_int rnd_color_load_int
#define pcb_color_load_packed rnd_color_load_packed
#define pcb_color_load_float rnd_color_load_float
#define pcb_color_load_str rnd_color_load_str
#define pcb_clrdup rnd_clrdup
#define pcb_color_init rnd_color_init
#define pcb_color_is_drill rnd_color_is_drill
#define pcb_clrcache_s rnd_clrcache_s
#define pcb_clrcache_init rnd_clrcache_init
#define pcb_clrcache_del rnd_clrcache_del
#define pcb_clrcache_get rnd_clrcache_get
#define pcb_clrcache_uninit rnd_clrcache_uninit
#define pcb_clrcache_t rnd_clrcache_t
#define pcb_clrcache_free_t rnd_clrcache_free_t
