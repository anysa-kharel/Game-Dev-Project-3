"""Audio preparation checks with editor stand-ins, not a real Unreal session."""
import importlib.util
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

class Sound:
    def __init__(self,loop=False,mode=0):self.props={'looping':loop,'virtualization_mode':mode,'volume':.37}
    def get_editor_property(self,name):return self.props[name]
    def set_editor_property(self,name,value):self.props[name]=value
class Task:
    def __init__(self):self.props={'imported_object_paths':[]}
    def get_editor_property(self,name):return self.props[name]
    def set_editor_property(self,name,value):self.props[name]=value

class AudioPreparation(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.assets={};self.imports=[];self.saved=[];self.import_ok=True
        def import_tasks(tasks):
            self.imports.extend(tasks)
            if self.import_ok:
                for t in tasks:
                    path=t.props['destination_path']+'/'+t.props['destination_name']
                    self.assets[path]=Sound();t.props['imported_object_paths']=[path]
        library=SimpleNamespace(does_asset_exist=lambda p:p in self.assets,load_asset=lambda p:self.assets.get(p),
            save_asset=Mock(side_effect=lambda p,**_:self.saved.append(p) or True))
        self.unreal=SimpleNamespace(EditorAssetLibrary=library,AssetImportTask=Task,log_warning=Mock(),
            VirtualizationMode=SimpleNamespace(PLAY_WHEN_SILENT=1),
            AssetToolsHelpers=SimpleNamespace(get_asset_tools=lambda:SimpleNamespace(import_asset_tasks=import_tasks)))
        self.modules=patch.dict(sys.modules,{'unreal':self.unreal});self.modules.start()
        path=Path(__file__).resolve().parents[1]/'Content/Python/build_arena.py'
        spec=importlib.util.spec_from_file_location('audio_prepare_test_target',path)
        self.module=importlib.util.module_from_spec(spec);spec.loader.exec_module(self.module)
        self.module._project_dir=lambda:self.root
    def tearDown(self):self.modules.stop();self.temp.cleanup()
    def existing(self,configured=True):
        for name,loop in self.module.PRESENTATION_AUDIO.items():
            self.assets[self.module.ASSET_ROOT+'/Audio/'+name]=Sound(loop if configured else False,1 if configured and loop else 0)
    def source_files(self):
        source=self.root/'Tools/AudioSource';source.mkdir(parents=True)
        for name in self.module.PRESENTATION_AUDIO:(source/(name+'.wav')).write_bytes(b'import task fixture')
    def test_new_sources_are_imported_and_music_is_configured(self):
        self.source_files();self.module._sounds()
        self.assertEqual(len(self.imports),3)
        for name,loop in self.module.PRESENTATION_AUDIO.items():
            sound=self.assets[self.module.ASSET_ROOT+'/Audio/'+name]
            self.assertEqual(sound.props['looping'],loop)
            if loop:self.assertEqual(sound.props['virtualization_mode'],1)
    def test_reopening_is_idempotent_and_keeps_effect_volumes(self):
        self.existing();self.source_files();self.module._sounds();self.module._sounds()
        self.assertEqual(self.imports,[]);self.assertEqual(self.saved,[])
        self.assertTrue(all(s.props['volume']==.37 for s in self.assets.values()))
    def test_existing_music_gets_looping_and_silent_playback(self):
        self.existing(False);self.module._sounds()
        self.assertEqual(len(self.saved),2)
        self.assertTrue(self.assets[self.module.ASSET_ROOT+'/Audio/S_MenuLoop'].props['looping'])
    def test_missing_required_audio_is_reported(self):
        with self.assertRaisesRegex(RuntimeError,'Missing required audio'):self.module._sounds()
    def test_import_failure_is_reported(self):
        self.source_files();self.import_ok=False
        with self.assertRaisesRegex(RuntimeError,'Missing required audio'):self.module._sounds()
    def test_failed_settings_save_is_reported(self):
        self.existing(False);self.unreal.EditorAssetLibrary.save_asset.side_effect=None
        self.unreal.EditorAssetLibrary.save_asset.return_value=False
        with self.assertRaisesRegex(RuntimeError,'Could not save audio settings'):self.module._sounds()
    def test_gate_cue_is_forced_to_one_shot(self):
        self.existing();path=self.module.ASSET_ROOT+'/Audio/S_GateOpen';self.assets[path].props['looping']=True
        self.module._sounds();self.assertFalse(self.assets[path].props['looping']);self.assertEqual(self.saved,[path])

if __name__=='__main__':unittest.main(verbosity=1)
