import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import hashlib
import launcher

class UpdateTests(unittest.TestCase):
    def test_download_validated_and_promoted(self):
        data=b'MZ'+b'example'*400
        manifest={'version':'0.2.0','url':'https://example.invalid','size':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        with tempfile.TemporaryDirectory() as folder,patch.object(launcher,'HOME',Path(folder)),patch('urllib.request.urlopen',return_value=io.BytesIO(data)):
            target=launcher.download_update(manifest,lambda _:None)
            self.assertEqual(target.read_bytes(),data)
            self.assertFalse(target.with_suffix('.part').exists())
    def test_corrupt_download_never_becomes_executable(self):
        data=b'MZ'+b'example'*400
        manifest={'version':'0.2.0','url':'https://example.invalid','size':len(data),'sha256':'0'*64}
        with tempfile.TemporaryDirectory() as folder,patch.object(launcher,'HOME',Path(folder)),patch('urllib.request.urlopen',return_value=io.BytesIO(data)):
            with self.assertRaises(ValueError):launcher.download_update(manifest,lambda _:None)
            self.assertFalse(list(Path(folder).rglob('*.exe')))
            self.assertFalse(list(Path(folder).rglob('*.part')))
    def test_oversized_download_rejected(self):
        manifest={'version':'0.2.0','url':'https://example.invalid','size':2000,'sha256':'0'*64}
        with tempfile.TemporaryDirectory() as folder,patch.object(launcher,'HOME',Path(folder)),patch('urllib.request.urlopen',return_value=io.BytesIO(b'MZ'+b'x'*3000)):
            with self.assertRaises(ValueError):launcher.download_update(manifest,lambda _:None)
    def test_numerical_version_comparison(self):
        self.assertGreater(launcher.version_key('0.10.0'),launcher.version_key('0.9.0'))

if __name__=='__main__':unittest.main()
