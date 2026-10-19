"""Run the actual compiled PS1 difficulty module under Unicorn (pip install unicorn).
Pass --symbols a JSON object from `mipsel-linux-gnu-nm build/mod/difficulty.elf`.
Game routines are stubbed; module instructions, state, pointers and ABI are real MIPS.
"""
import argparse,json,struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_MIPS, UC_MODE_MIPS32, UC_MODE_LITTLE_ENDIAN, UC_HOOK_CODE
from unicorn.mips_const import *
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--symbols',required=True)
p.add_argument('--module',default=str(Path(__file__).resolve().parents[2]/'build/mod/kit/DIFF.BIN'))
args=p.parse_args()
s=json.loads(Path(args.symbols).read_text())
u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_LITTLE_ENDIAN);u.mem_map(0,0x200000)
u.mem_write(0x10c000,Path(args.module).read_bytes())
actor,outer,init,record=0x80170000,0x80160000,0x80150000,0x80180000
physical=lambda a:a&0x1fffffff

def write(a,data): u.mem_write(physical(a),bytes(data))
def read(a,size): return bytes(u.mem_read(physical(a),size))
def word(a,value=None):
    if value is not None:write(a,struct.pack('<I',value))
    return struct.unpack('<I',read(a,4))[0]
def half(a,value=None):
    if value is not None:write(a,struct.pack('<H',value))
    return struct.unpack('<H',read(a,2))[0]
status_seen=[]

def game_hook(uc,address,size,data):
    if address==s['func_80076784']:
        write(u.reg_read(UC_MIPS_REG_A1)+0x957,bytes([u.reg_read(UC_MIPS_REG_A0)]))
    elif address==s['func_80069FC4']:
        b=bytearray(40);b[0]=1
        values=[half(actor+i) for i in (0x18,0x1a,0x1c,0x1e,0x24,0x28,0x2c)]
        values += [half(actor+0x398+i*0xdc) for i in range(6)]
        struct.pack_into('<13H',b,4,*values);write(record,b)
    elif address==s['func_80069E80']:
        u.reg_write(UC_MIPS_REG_V0,record)
    elif address==s['func_80086FA8']:
        status_seen.append(half(actor+0x22))
    elif address!=s['func_80069DBC']:
        return
    u.reg_write(UC_MIPS_REG_PC,u.reg_read(UC_MIPS_REG_RA))
u.hook_add(UC_HOOK_CODE,game_hook)

def call(name,*arguments):
    u.reg_write(UC_MIPS_REG_SP,0x801fe000);u.reg_write(UC_MIPS_REG_RA,0x80010000)
    for reg,value in zip((UC_MIPS_REG_A0,UC_MIPS_REG_A1,UC_MIPS_REG_A2,UC_MIPS_REG_A3),arguments):u.reg_write(reg,value)
    saved=[UC_MIPS_REG_S0,UC_MIPS_REG_S1,UC_MIPS_REG_S2,UC_MIPS_REG_S3,UC_MIPS_REG_S4,UC_MIPS_REG_S5,UC_MIPS_REG_S6,UC_MIPS_REG_S7]
    for i,reg in enumerate(saved):u.reg_write(reg,0x12340000+i)
    u.emu_start(s[name],0x80010000,count=1000000)
    assert u.reg_read(UC_MIPS_REG_PC)==0x80010000,name
    assert all(u.reg_read(reg)==0x12340000+i for i,reg in enumerate(saved)),name+' clobbered saved register'
    assert u.reg_read(UC_MIPS_REG_SP)==0x801fe000,name+' unbalanced stack'

def settings(base,per=0,clears=2):
    word(0x8010c00c,base);word(0x8010c010,per);write(s['vs_main_stateFlags'],bytes([clears]));call('vs_diff_start')

def fresh(slot=2,model=12,hp=100):
    write(actor,bytes(0x958));write(outer,bytes(0x50));write(init,bytes(0x100))
    word(s['vs_battle_actors']+slot*4,outer);word(outer+0x3c,actor)
    write(outer+0x18,bytes([model]));write(outer+0x27,bytes([0]));write(init+2,bytes([model]))
    for offset,value in [(0x18,hp),(0x1a,hp),(0x22,100),(0x24,100),(0x26,101),(0x28,101),(0x2a,999),(0x2c,999)]:half(actor+offset,value)
    for i in range(5):half(actor+0x398+i*0xdc,200);half(actor+0x39a+i*0xdc,200)
    call('vs_diff_init',slot,actor,init,0)

def restore_raw():
    values=struct.unpack_from('<13H',read(record,40),4)
    for offset,value in zip((0x18,0x1a,0x1c,0x1e),values[:4]):half(actor+offset,value)
    for offset,value in zip((0x22,0x26,0x2a),values[4:7]):half(actor+offset,value);half(actor+offset+2,value)
    for i,value in enumerate(values[7:]):half(actor+0x398+i*0xdc,value)
    call('vs_diff_restore',0,actor)

settings(30,30);fresh()
assert half(actor+0x1a)==190 and half(actor+0x22)==190 and half(actor+0x26)==191
assert half(actor+0x2a)==999 and half(actor+0x39a)==380 and half(actor+0x1e)==0
half(actor+0x18,165);half(actor+0x398,373);call('vs_diff_save',2,0)
assert half(record+6)==100 and half(record+12)==100 and half(record+14)==101
for _ in range(10):
    fresh(hp=105);restore_raw()
    assert half(actor+0x18)==165 and half(actor+0x1a)==190 and half(actor+0x39a)==380 and half(actor+0x398)==373
    assert status_seen[-1]==190
    call('vs_diff_save',2,0)
settings(30,30);fresh(hp=110);restore_raw();call('vs_diff_save',2,0)
hp,limb=half(record+4),half(record+18)
for _ in range(10):
    settings(30,30);fresh(hp=110);restore_raw();call('vs_diff_save',2,0)
    assert half(record+4)==hp and half(record+18)==limb
settings(0);fresh();restore_raw();assert half(actor+0x1a)==100 and half(actor+0x22)==100
settings(30,30)
for slot,model in [(0,0),(7,8)]:
    fresh(slot,model);assert half(actor+0x1a)==100 and half(actor+0x22)==100
settings(100);fresh(hp=800)
assert half(actor+0x1a)==999 and half(actor+0x39a)==250
print('PASS actual MIPS: additive scaling, caps, limb ratios, exclusions, exact cached revisits, stable card conversions, restoration before statuses, saved-register and stack preservation')
