#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# ========================================================================
# 이 스크립트는 프로파일링 결과 파일(profile_data.txt)을 읽어들여
# 스레드별 평균 수행 시간을 그래프로 시각화하여 이미지로 저장합니다.
# - 자동으로 3가지 단위(ns, μs, ms) 모두 실행하여 각각 이미지를 생성합니다.
# ========================================================================

import sys
import subprocess
from pathlib import Path

# ──────────────────────────────────────────────────────────────
# 1) matplotlib 설치 및 import
#    - 그래프 생성을 위해 사용합니다.
#    - 설치되어 있지 않으면 pip를 통해 자동 설치합니다.
try:
    import matplotlib
    import matplotlib.pyplot as plt
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "matplotlib"])
    import matplotlib
    import matplotlib.pyplot as plt

# ──────────────────────────────────────────────────────────────
# 2) pandas, numpy 설치 및 import
#    - 데이터 처리 및 수치 계산을 위해 사용합니다.
try:
    import pandas as pd
    import numpy as np
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pandas numpy"])
    import pandas as pd
    import numpy as np

# ──────────────────────────────────────────────────────────────
# 3) 한글 폰트 설정
#    - Windows 환경의 Malgun Gothic 예시
matplotlib.rc('font', family='Malgun Gothic', size=10)
matplotlib.rc('axes', unicode_minus=False)
# ──────────────────────────────────────────────────────────────

def plot_average_per_thread(txt_path: Path, out_image: Path, unit: str):
    """
    프로파일 텍스트 파일을 읽어들여
    스레드별 평균 수행 시간 그래프를 생성하고 저장합니다.

    Args:
        txt_path: profile_data.txt 경로
        out_image: 출력 이미지 베이스 경로
        unit: 'ns', 'us', 'ms'
    """

    # 1) 파일 읽기 및 파싱
    with txt_path.open(encoding='utf-8') as f:
        lines = [l.rstrip('\n') for l in f]
    headers = [h.strip() for h in lines[0].split('|')]
    records = []
    for line in lines[2:]:
        if not line.strip():
            continue
        parts = [p.strip() for p in line.split('|')]
        if len(parts) == len(headers):
            records.append(dict(zip(headers, parts)))
    df = pd.DataFrame(records)
    df['Average'] = df['Average'].astype(float)  # 기본 ms 단위

    # 2) Threads 및 Op 컬럼 생성
    df['Threads'] = df['Name'].str.extract(r'^(\d+)').astype(int)
    df['OpRaw'] = (
        df['Name']
        .str.extract(r'threads\s+([A-Za-z]+)')[0]
        .str.replace(r'\d+', '', regex=True)
        .str.lower()
    )
    op_map = {'new': 'malloc', 'alloc': 'TLSAlloc', 'delete': 'free', 'free': 'TLSFree'}
    df['Op'] = df['OpRaw'].map(op_map)

    # 3) 피벗 테이블
    pivot = (
        df.pivot(index='Threads', columns='Op', values='Average')
          .sort_index()
    )

    # 4) 단위 변환 설정
    unit = unit.lower()
    if unit == 'ns':
        factor = 1e6
        ylabel = '평균 수행 시간 (ns)'
        suffix = '_ns'
    elif unit == 'us':
        factor = 1e3
        ylabel = '평균 수행 시간 (μs)'
        suffix = '_us'
    else:
        factor = 1
        ylabel = '평균 수행 시간 (ms)'
        suffix = '_ms'
    pivot_scaled = pivot * factor

    # 5) 그래프 그리기
    threads = pivot_scaled.index.tolist()
    ops = ['malloc', 'TLSAlloc', 'free', 'TLSFree']
    bar_w = 0.2
    x = np.arange(len(threads))

    fig, ax = plt.subplots(figsize=(10, 6))
    max_val = pivot_scaled.max().max()
    offset = max_val * 0.005

    for i, op in enumerate(ops):
        if op not in pivot_scaled.columns:
            continue
        vals = pivot_scaled[op]
        bars = ax.bar(x + i*bar_w, vals, width=bar_w, label=op)
        for b in bars:
            h = b.get_height()
            if unit == 'ms':
                text = f'{h:.6f}'
            elif unit == 'us':
                text = f'{h:.3f}'
            else:
                text = f'{h:.0f}'
            ax.text(
                b.get_x() + b.get_width()/2,
                h + offset,
                text,
                ha='center', va='bottom', fontsize=8
            )

    # 6) 레이아웃 설정
    ax.set_ylim(0, max_val*1.15)
    ax.set_xlabel('스레드 수')
    ax.set_ylabel(ylabel)
    ax.set_title('스레드별 평균 수행 시간 비교\n(malloc vs TLSAlloc, free vs TLSFree)')
    ax.set_xticks(x + 1.5*bar_w)
    ax.set_xticklabels(threads)
    ax.legend()
    plt.tight_layout()

    # 7) 이미지 저장
    out_path = out_image.with_name(out_image.stem + suffix + out_image.suffix)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, dpi=300)
    print(f"'{out_path.name}' 생성 완료")


if __name__ == '__main__':
    # 실행 기준 디렉터리
    base_dir = Path(__file__).parent
    txt_file = base_dir / 'profile_data.txt'
    img_file = base_dir / 'comparison_average_per_thread.png'

    # 입력 파일 확인
    if not txt_file.exists():
        print(f"ERROR: '{txt_file}' 파일이 없습니다.")
        sys.exit(1)

    # 3가지 단위로 자동 실행
    for unit in ['ns', 'us', 'ms']:
        plot_average_per_thread(txt_file, img_file, unit)
