import uproot
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

import glob
import sys

df_evt=pd.DataFrame(columns=['tx','ty','tz'])
if len(sys.argv)==2:
    evtpath=sys.argv[1]
    df_evt=pd.read_csv(sys.argv[1])
df_evt['tr']=np.sqrt(df_evt.tx*df_evt.tx+df_evt.ty*df_evt.ty)

df=uproot.open('./hits.root')['hits'].arrays(library='pd')
df['tr']=np.sqrt(df.tx*df.tx+df.ty*df.ty)

for volume_id,vdf in df.groupby('volume_id'):
    plt.plot(vdf.tz,vdf.tr,'.',label='{}'.format(volume_id))
plt.plot(df_evt.tz,df_evt.tr,'k*')
#plt.xlim(-3200,3200)
plt.xlim(-3000,3000)
plt.ylim(    0,1500)
#plt.ylim(    0,3200)
plt.xlabel('z [mm]')
plt.ylabel('r [mm]')
plt.legend(title='ATLAS Inner Detector: ACTS volume_id',ncol=2)
#plt.text(2,  0.5, 'CEPC 4^{th} concept')
plt.tight_layout()
plt.savefig('volume_id.png')
plt.show()
plt.clf()

for layer_id,ldf in df.groupby('layer_id'):
    plt.plot(ldf.tz,ldf.tr,'.',label='{}'.format(layer_id))
plt.plot(df_evt.tz,df_evt.tr,'k*')
plt.xlim(0,3200)
plt.ylim(0,3200)
plt.xlabel('z [mm]')
plt.ylabel('r [mm]')
plt.legend(title='layer_id',ncol=2)
plt.tight_layout()
plt.savefig('layer_id.png')
plt.show()
plt.clf()

df_barrel=df[df.volume_id.isin([14,17,20,24])]
for layer_id,sdf in df_barrel.groupby('layer_id'):
    plt.plot(sdf.tx,sdf.ty,'.',label=f'{layer_id}')
plt.plot(df_evt.tx,df_evt.ty,'k*')
plt.xlim(0,3200)
plt.ylim(0,3200)
plt.xlabel('x [mm]')
plt.ylabel('y [mm]')
#plt.legend(title='layer_id')
plt.title('Barrel')
plt.tight_layout()
plt.savefig('layer_id.png')
plt.show()
plt.clf()
