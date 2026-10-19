import contextlib,io,tempfile,unittest,sys,struct
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parent))
import patch_disc,patch_image
class DifficultyOptions(unittest.TestCase):
 def test_prompt_validation_and_remembered_choices(self):
  with tempfile.TemporaryDirectory() as folder, contextlib.redirect_stdout(io.StringIO()):
   with patch('builtins.input',side_effect=['','-1','10001','30','bad','30','2','y']):
    choice=patch_disc.choose_options(folder)
   self.assertEqual(choice,(['max_stat_items_zenith','choosable_bonus_wheel'],30,30))
   with patch('builtins.input',return_value='y'):
    self.assertEqual(patch_disc.choose_options(folder),choice)
 def test_old_ini_requires_new_choices(self):
  with tempfile.TemporaryDirectory() as folder:
   Path(folder,'Merlose options.ini').write_text('[features]\nmax_stat_items = 4\nchoosable_bonus_wheel = yes\n')
   self.assertEqual(patch_disc.saved_options(folder),{})
 def test_cli_and_headers(self):
  self.assertEqual(patch_image.difficulty_options(['--difficulty-base=30','--difficulty-per-clear=30']),[30,30])
  for bad in ['-1','1.5','10001','']:
   with self.assertRaises(SystemExit):patch_image.difficulty_options(['--difficulty-base='+bad])
  b=bytearray(32);struct.pack_into('<I',b,0,0x46464944)
  configured=patch_image.configure_difficulty('DIFF.BIN',b,True,30,40)
  self.assertEqual(struct.unpack_from('<II',configured,12),(30,40))
  self.assertEqual(struct.unpack_from('<I',patch_image.configure_difficulty('CALLO.BIN',bytes(32),True,30,40),12)[0],0x44494646)
if __name__=='__main__':unittest.main()
