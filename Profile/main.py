#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import subprocess
from pathlib import Path

# ──────────────────────────────────────────────────────────────
# matplotlib 설치 & 불러오기
try:
    import matplotlib
    import matplotlib.pyplot as plt
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "matplotlib"])
    import matplotlib
    import matplotlib.pyplot as plt

# pandas, numpy 설치 & 불러오기
try:
    import pandas as pd
    import numpy as np
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pandas numpy"])
    import pandas as pd
    import numpy as np

# 한글 폰트 설정 (Windows Malgun Gothic 예시)
matplotlib.rc('font', family='Malgun Gothic', size=10)
matplotlib.rc('axes', unicode_minus=False)
# ──────────────────────────────────────────────────────────────

def plot_total_per_thread(txt_path: Path, out_image: Path):
    # 1) 텍스트 파싱
    with txt_path.open(encoding='utf-8') as f:
        lines = [l.rstrip('\n') for l in f]
    headers = [h.strip() for h in lines[0].split('|')]
    records = []
    for line in lines[2:]:
        if not line.strip(): continue
        parts = [p.strip() for p in line.split('|')]
        if len(parts) == len(headers):
            records.append(dict(zip(headers, parts)))

    df = pd.DataFrame(records)
    df['Total'] = df['Total'].astype(float)

    # 2) 스레드 수, 연산명 추출 및 매핑
    df['Threads'] = df['Name'].str.extract(r'^(\d+)').astype(int)
    df['OpRaw']   = (
        df['Name']
        .str.extract(r'threads\s+([A-Za-z]+)')[0]
        .str.replace(r'\d+', '', regex=True)
        .str.lower()
    )
    op_map = {'new': 'malloc', 'alloc': 'TLSAlloc', 'delete': 'free', 'free': 'TLSFree'}
    df['Op'] = df['OpRaw'].map(op_map)

    # 3) 스레드별 1스레드당 Total 시간 계산
    df['TotalPerThread'] = df['Total'] / df['Threads']

    # 4) Pivot 테이블 생성 (index=Threads, columns=Op, values=TotalPerThread)
    pivot = (
        df.pivot(index='Threads', columns='Op', values='TotalPerThread')
          .sort_index()
    )

    # 5) 밀리초 단위 변환 (초 → ms)
    pivot_ms = pivot * 1e3

    # 6) 막대그래프 그리기
    threads = pivot_ms.index.to_list()
    ops     = ['malloc', 'TLSAlloc', 'free', 'TLSFree']
    bar_w   = 0.2
    x       = np.arange(len(threads))

    fig, ax = plt.subplots(figsize=(10, 6))
    max_val = pivot_ms.max().max()
    for i, op in enumerate(ops):
        if op not in pivot_ms.columns:
            continue
        vals = pivot_ms[op]
        bars = ax.bar(x + i*bar_w, vals, width=bar_w, label=op)
        for b in bars:
            h = b.get_height()
            ax.text(
                b.get_x() + b.get_width()/2,
                h + max_val*0.01,
                f'{h:.1f}',
                ha='center', va='bottom'
            )

    ax.set_xlabel('스레드 갯수')
    ax.set_ylabel('1스레드당 총 수행 시간 (ms)')
    ax.set_title('스레드별 1스레드당 Total 시간 비교\n(malloc vs TLSAlloc, free vs TLSFree)')
    ax.set_xticks(x + 1.5*bar_w)
    ax.set_xticklabels(threads)
    ax.legend()
    plt.tight_layout()

    # 7) 이미지 저장
    out_image.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_image, dpi=300)
    print(f"'{out_image.name}' 생성 완료: {out_image}")

if __name__ == '__main__':
    base_dir = Path(__file__).parent
    txt_file  = base_dir / 'profile_data.txt'
    img_file  = base_dir / 'comparison_total_per_thread.png'

    if not txt_file.exists():
        print(f"ERROR: '{txt_file}' 파일이 없습니다.")
        sys.exit(1)

    plot_total_per_thread(txt_file, img_file)
