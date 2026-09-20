"""Guard the two web-only failures a native executable cannot catch."""
import importlib.util
import argparse
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("web_wrapper", ROOT / "tools/web/wrap.py")
wrapper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wrapper)

class WebWrapperTests(unittest.TestCase):
    def render(self, source_bytes, title="Test", arguments=None):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder) / "module.js"
            output = Path(folder) / "nested" / "play.html"
            source.write_bytes(source_bytes)
            size = wrapper.wrap(source, output, title, arguments)
            page = output.read_bytes()
            self.assertEqual(size, len(page))
            return page

    def test_utf8_and_raw_module_newlines_survive(self):
        # Some SINGLE_FILE versions encode WASM in a string with unusual bytes.
        source = b'/* \r\nraw:\xff */\nvar RL = () => Promise.resolve({});\r'
        page = self.render(source)
        self.assertIn(b'<meta charset="utf-8">', page)
        self.assertIn(b"<script>" + source + b"</script>", page)
        self.assertIn(b"height:100%;", page)
        self.assertNotIn(b"100%%", page)

    def test_embedded_script_end_cannot_close_the_html_element(self):
        page = self.render(b'var payload = "</ScRiPt><h1>oops</h1>";')
        self.assertIn(b'"<\\/ScRiPt><h1>oops</h1>"', page)
        self.assertEqual(page.lower().count(b"</script>"), 2)

    def test_title_is_escaped_and_module_failure_is_observable(self):
        page = self.render(b"var RL = () => Promise.reject('test');", "<script>&")
        self.assertIn(b"<title>&lt;script&gt;&amp;</title>", page)
        self.assertIn(b"window.__moduleError = String(error)", page)
        self.assertNotIn(b"src=", page)

    def test_developer_arguments_are_json_data_not_script_markup(self):
        arguments = ['--room', '1', '--at', '22,3', '--mute', '\"</script><script>alert(1)</script>\n\\']
        page = self.render(b"var RL = () => Promise.resolve({});", arguments=arguments)
        encoded = json.dumps(arguments, ensure_ascii=True).replace("<", "\\u003c").encode()
        self.assertIn(b"arguments: " + encoded, page)
        self.assertEqual(page.lower().count(b"</script>"), 2)
        self.assertEqual(wrapper.parse_arguments(json.dumps(arguments)), arguments)

    def test_developer_arguments_reject_non_string_arrays(self):
        for value in ('{}', 'null', '[1]', '[true]', '[', '"--room"'):
            with self.assertRaises(argparse.ArgumentTypeError):
                wrapper.parse_arguments(value)
        self.assertIn(b"arguments: []", self.render(b"var RL;"))

if __name__ == "__main__":
    unittest.main()
