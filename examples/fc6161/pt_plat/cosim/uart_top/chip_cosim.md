# uart_top Chip-Level CModel Co-Simulation

## Prerequisites
- gen-model이 llhd.sig를 지원하여 완전한 CModel 생성
- VCS 라이센스 사용 가능 (chip-level TB는 VCS 전용)

## Concept
fc6161의 chip-level TB(fc6161_lhotse_tb)에서:
- uart_top RTL filelist 대신 DPI-C wrapper filelist 사용
- 나머지 모듈(s5, gpio, clk_rst 등)은 원본 RTL
- chip-level 통합 시뮬레이션 실행

## When Ready
1. gen-model의 llhd.sig 지원 완료
2. VCS 라이센스 데몬 업그레이드 (SCL 2024.06+)
3. `make cosim-uart-chip-vcs` 타겟 활성화
