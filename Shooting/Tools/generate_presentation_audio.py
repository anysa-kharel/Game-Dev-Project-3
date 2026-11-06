"""Recreate Tick Tock music, gate cue and reload effect (requires NumPy/SciPy).

The supplied WAVs are ready to import; running this generator is optional.
No recordings, external samples, vocals, or third-party music are used.
"""
from pathlib import Path
import json
import wave
import numpy as np
from scipy.signal import butter, sosfilt

RATE=32000
ROOT=Path(__file__).resolve().parent/'AudioSource'
RNG=np.random.default_rng(7721)

def note(midi):return 440*2**((midi-69)/12)
def add_loop(bus,start,voice,pan=0):
    pos=(np.arange(len(voice))+round(start*RATE))%len(bus)
    angle=(pan+1)*np.pi/4
    np.add.at(bus[:,0],pos,voice*np.cos(angle))
    np.add.at(bus[:,1],pos,voice*np.sin(angle))
def pad(freq,duration,amount):
    t=np.arange(round(duration*RATE))/RATE
    envelope=np.sin(np.pi*np.clip(t/duration,0,1))**1.8
    tone=(.7*np.sin(2*np.pi*freq*t)+.2*np.sin(2*np.pi*freq*2*t+.03*np.sin(2*np.pi*.2*t))
          +.07*np.sin(2*np.pi*freq*3*t))
    return amount*tone*envelope
def pluck(freq,duration,amount):
    t=np.arange(round(duration*RATE))/RATE
    envelope=(1-np.exp(-t*45))*np.exp(-t*3.0)*np.minimum(1,(duration-t)*15)
    return amount*envelope*(np.sin(2*np.pi*freq*t)+.16*np.sin(2*np.pi*freq*2*t))
def menu():
    bus=np.zeros((24*RATE,2))
    chords=((38,45,53,57),(34,41,50,53),(31,43,50,58),(33,45,52,56))
    for i,chord in enumerate(chords):
        for j,m in enumerate(chord):add_loop(bus,i*6-1.1,pad(note(m),7.6,.11 if j else .15),(j-1.5)*.25)
    melody=(62,57,65,69,58,65,62,65,67,62,70,65,69,64,68,61)
    for i,m in enumerate(melody):add_loop(bus,i*1.5+.3,pluck(note(m),2.4,.11),(-1 if i%2 else 1)*.35)
    for i in range(32):add_loop(bus,i*.75,pluck(680 if i%2 else 520,.14,.021),(-1 if i%2 else 1)*.15)
    return bus+.12*np.roll(bus,round(.375*RATE),axis=0)+.055*np.roll(bus,round(.75*RATE),axis=0)
def gameplay():
    bus=np.zeros((32*RATE,2))
    chords=((38,45,53),(36,43,52),(34,41,50),(33,40,49))
    for i,chord in enumerate(chords):
        for j,m in enumerate(chord):add_loop(bus,i*8-1.4,pad(note(m),10.8,.12 if j else .16),(j-1)*.45)
    for i in range(16):add_loop(bus,i*2+.3,pluck(290 if i%2 else 220,.7,.013),(-1 if i%2 else 1)*.3)
    return bus+.13*np.roll(bus,round(.56*RATE),axis=0)
def gate():
    duration=1.6;t=np.arange(round(duration*RATE))/RATE;out=np.zeros(len(t))
    for start in (0,.075):
        q=np.maximum(0,t-start);env=(t>=start)*(1-np.exp(-q*450))*np.exp(-q*38)
        out+=env*(.32*np.sin(2*np.pi*520*q)+.16*np.sin(2*np.pi*1170*q))
    motor=((t>.09)&(t<1.05))*np.sin(np.pi*np.clip((t-.09)/.96,0,1))**1.2
    out+=motor*(.08*np.sin(2*np.pi*(135*t+75*t*t))+.035*np.sin(2*np.pi*360*t))
    noise=sosfilt(butter(3,[300,1500],fs=RATE,btype='bandpass',output='sos'),RNG.normal(0,1,len(t)))
    out+=motor*noise*.055
    q=np.maximum(0,t-1.03);out+=(t>=1.03)*np.exp(-q*19)*(1-np.exp(-q*200))*(.18*np.sin(2*np.pi*180*q)+.08*np.sin(2*np.pi*620*q))
    for start,f in ((1.1,587.33),(1.23,880)):
        v=pluck(f,min(.35,duration-start),.04);at=round(start*RATE);out[at:at+len(v)]+=v
    out[:320]*=np.linspace(0,1,320);out[-320:]*=np.linspace(1,0,320)
    return out
def reload_sound():
    """Magazine release, slide, insertion and bolt latch; no external samples."""
    duration=1.42;t=np.arange(round(duration*RATE))/RATE;out=np.zeros(len(t))
    local_rng=np.random.default_rng(48271)
    noise=local_rng.normal(0,1,len(t))
    metal=sosfilt(butter(3,[550,5500],fs=RATE,btype='bandpass',output='sos'),noise)
    scrape=sosfilt(butter(3,[350,2600],fs=RATE,btype='bandpass',output='sos'),noise)
    for start,amount,frequency,decay in ((.018,.22,1680,58),(.12,.10,820,42),
                                        (.44,.07,1200,60),(.78,.28,270,26),
                                        (1.03,.14,880,45),(1.20,.21,1400,54)):
        q=np.maximum(0,t-start)
        envelope=(t>=start)*(1-np.exp(-q*900))*np.exp(-q*decay)
        out+=amount*envelope*(.65*metal+.5*np.sin(2*np.pi*frequency*q)
                              +.18*np.sin(2*np.pi*frequency*1.71*q))
    for start,length,amount in ((.14,.20,.07),(.57,.20,.055),(.91,.15,.085),(1.09,.10,.07)):
        phase=np.clip((t-start)/length,0,1)
        out+=amount*scrape*np.sin(np.pi*phase)**1.3
    # Round transient peaks so the mechanical details are audible at modest gain.
    out=np.tanh(out*16)
    out[:160]*=np.linspace(0,1,160);out[-640:]*=np.linspace(1,0,640)
    return out
def save(name,samples,rms):
    samples=samples-np.mean(samples,axis=0)
    gain=min(rms/max(1e-9,np.sqrt(np.mean(samples*samples))),.62/max(1e-9,np.max(np.abs(samples))))
    samples=samples*gain
    pcm=np.round(np.clip(samples,-1,1)*32767).astype('<i2')
    ROOT.mkdir(parents=True,exist_ok=True)
    with wave.open(str(ROOT/(name+'.wav')),'wb') as f:
        f.setnchannels(1 if samples.ndim==1 else samples.shape[1]);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(pcm.tobytes())
    return {'file':name+'.wav','seconds':len(samples)/RATE,'channels':1 if samples.ndim==1 else samples.shape[1],
            'peak_dbfs':round(20*np.log10(max(np.max(np.abs(samples)),1e-12)),2),
            'rms_dbfs':round(20*np.log10(np.sqrt(np.mean(samples*samples))),2),
            'boundary_step':float(np.max(np.abs(samples[-1]-samples[0])))}
def main():
    results=[save('S_MenuLoop',menu(),.16),save('S_GameAmbience',gameplay(),.12),
             save('S_GateOpen',gate(),.09),save('S_Reload',reload_sound(),.14)]
    print(json.dumps(results,indent=2))
if __name__=='__main__':main()
