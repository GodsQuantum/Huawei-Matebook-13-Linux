#!/usr/bin/env python3
"""Compile actual C second-response timeout selection without any sensor IO."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
SOURCE=(ROOT/'driver/goodix51a0/goodix51a0.c').read_text()
START='static int\ngx_fdt_down_second_reply_poll_limit (void)'
END='\nstatic int\ng'
assert START in SOURCE, 'qa helper must exist in compiled driver'
a=SOURCE.index(START)
b=SOURCE.index('\n}',a)+2
FUNCTION=SOURCE[a:b]
assert 'GXFP_FDT_DOWN_SECOND_REPLY_WINDOW_QA_MS' in FUNCTION
assert 'GX_FDT_DOWN_SECOND_REPLY_POLLS' in FUNCTION
assert 'gx_fdt_down_second_reply_poll_limit ()' in SOURCE[b:][0:16000]
C=r"""
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GX_DRAIN_IRQ_POLL_MS 10
typedef char gchar;
#define GX_FDT_DOWN_SECOND_REPLY_TIMEOUT_MS 100
#define GX_FDT_DOWN_SECOND_REPLY_POLLS 10
static const char *g_getenv(const char *key){return getenv(key);}
static int g_strcmp0(const char *a,const char *b)
{return a==NULL?-1:strcmp(a,b);}
""" + FUNCTION + r"""
int main(int argc,char**argv)
{
 if(argc != 2)return 90;
 if(strcmp(argv[1],"unset")==0)unsetenv("GXFP_FDT_DOWN_SECOND_REPLY_WINDOW_QA_MS");
 else setenv("GXFP_FDT_DOWN_SECOND_REPLY_WINDOW_QA_MS",argv[1],1);
 return gx_fdt_down_second_reply_poll_limit();
}
"""
class TimeoutSelection(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory(prefix='gxfp_fdt_window_')
  cpp=Path(cls.tmp.name)/'timeout.c';cpp.write_text(C)
  cls.exe=Path(cls.tmp.name)/'timeout'
  subprocess.run(['cc','-O2','-Wall','-Wextra','-Werror','-pedantic','-std=c11',str(cpp),'-o',str(cls.exe)],check=True,capture_output=True)
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def check(self,mode,n):self.assertEqual(subprocess.run([str(self.exe),mode]).returncode,n)
 def test_default_100ms(self):self.check('unset',10)
 def test_200ms_optin(self):self.check('200',20)
 def test_300ms_optin(self):self.check('300',30)
 def test_disallowed_values_fall_back(self):
  for v in ('0','10','50','1000','999999','bogus',''):
   with self.subTest(v=v):self.check(v,10)
 def test_mandatory_fdt_two_reply_proof_unmodified(self):
  self.assertIn('expected_plain_replies = 2; break; /* FDT-down ACK + data */',SOURCE)
  self.assertIn('got < expected_plain_replies || !ack_seen',SOURCE)
  self.assertIn('gx_capture_transport_desync (self);',SOURCE)
if __name__=='__main__':unittest.main()
