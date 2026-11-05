"""Prepare the included native level once the Unreal Editor is ready."""
import traceback
import unreal

_arena_handle = None


def _arena_first_tick(_delta):
    global _arena_handle
    if unreal.AssetRegistryHelpers.get_asset_registry().is_loading_assets():
        return
    if _arena_handle is not None:
        unreal.unregister_slate_post_tick_callback(_arena_handle)
        _arena_handle = None
    try:
        import build_arena
        build_arena.prepare()
    except Exception:
        unreal.log_error(
            "Clock Arena content preparation did not finish. "
            "The C++ runtime fallback can still build the level when you press Play. "
            "Use Tools > Execute Python Script > Content/Python/build_arena.py "
            "to retry. Details:\n" + traceback.format_exc()
        )


_arena_handle = unreal.register_slate_post_tick_callback(_arena_first_tick)
