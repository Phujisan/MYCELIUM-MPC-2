#!/usr/bin/env python3
import json, pathlib, re
root=pathlib.Path(__file__).resolve().parents[1]
man=json.loads((root/'plugin.json').read_text())
ui=json.loads((root/'ui.json').read_text())
src=(root/'src/MYCELIUM.cpp').read_text()
assert man['file64']=='VST/mycelium-arm64.so'
assert man['uid']=='My98'
assert len(ui['pages'])==12
params=re.findall(r'addParam\(mpc::(?:floatParam|logParam|boolParam|choiceParam)\("([^"]+)", "([^"]+)"',src)
assert len(params)==98, len(params)
assert len(set(k for k,_ in params))==98
names={name for _,name in params}
all_ui=[]
for page in ui['pages']:
  assert len(page['name'])<=8
  assert len(page['controls'])<=16
  for c in page['controls']:
    assert c['param'] in names,(page['name'],c)
    assert c['widget'] in ('knob','fader','switch','slider')
    all_ui.append(c['param'])
for label in ('Freeze','Time Speed','Time Mode','Fresh Notes','Grain Explore','Loop Explore','Master Level'):
  assert label in all_ui,label
print('PASS: 98 stable parameters, 12 MPC pages, touchscreen controls and Q-Link mappings')
