# stm32f411-uart-pwm-motor-control
> STM32F411 motor controller: Duty cycle control via UART interrupts &amp; direction/state control via GPIO button

# STM32F411 DC Motor Control System (REV 1.3)

> **CMSIS 및 레지스터 직접 제어 기반으로 구현한 인터럽트 기반 DC 모터 제어 펌웨어**

---
## 📑 목차 (Table of Contents)
- [1. 개발 환경 및 기술 스택](#개발-환경-및-기술-스택)
- [2. 주요 기능 및 설계 특징](#주요-기능-및-설계-특징)
- [3. 하드웨어 핀 맵 (Pin Map)](#하드웨어-핀-맵)
- [4. 모터 상태 전이 시나리오 (State Machine)](#모터-상태-전이-시나리오-state-machine)
- [5. 파일 구조 및 역할](#파일-구조-및-역할)
- [6. 주요 트러블슈팅 및 문제 해결 (Troubleshooting)](#주요-트러블슈팅-및-문제-해결)
- [7. REV 1.3 주요 변경 사항](#rev-13-주요-변경-사항)
- [8. 향후 개선 사항](#향후-개선-사항)

---

## 개발 환경 및 기술 스택

* **Target MCU:** STM32F411 (ARM Cortex-M4 Core)
* **Library:** ARM CMSIS Core (`core_cm4.h`), STMicroelectronics Device Header (`stm32f411xe.h`)
* **Build System & Toolchain:** GNU Toolchain (`arm-none-eabi-gcc`), Makefile, Linker Script (`.lds`), Assembly (`crt0.s`)
* **Core Architecture:** Non-blocking Interrupt-driven Architecture

---

## 주요 기능 및 설계 특징

* **인터럽트 기반 논블로킹 아키텍처:** EXTI(버튼), USART2(UART RX), TIM2/TIM4(타이머) 모두 인터럽트로 처리, 메인 루프는 플래그 체크만 수행
* **버튼(PC13) 동작 제어**
  * Short Press: 방향 토글 (`STOP → CW → CCW → CW → ...`)
  * Short Stop 지연: 방향 전환 시 200ms 정지 후 반대 방향 구동
  * Long Press (3초 이상): 하드웨어 타이머 타임아웃 기반 강제 정지
* **UART2 기반 실시간 속도 제어:** 115200bps, `'1'`~`'9'` 수신 시 속도 테이블에 따라 TIM5 PWM Duty 즉시 변경
* **FPU & PWM 최적화:** `float`(32bit) 연산 전환으로 FPU 미지원 연산 오버헤드 제거, TIM5 PWM Mode 1 기반 모터 구동

---

## 하드웨어 핀 맵

| 기능 | Pin | 제어 방식 / 모드 |
|---|---|---|
| 모터 방향 버튼 | `PC13` | EXTI13 (Rising / Falling Edge Interrupt) |
| 모터 PWM 출력 (CW/CCW) | `PA0`, `PA1` | TIM5 CH1/CH2 PWM, GPIO 모드 전환 |
| UART 통신 | `PA2`(TX), `PA3`(RX) | USART2 (115200bps, RX Interrupt) |

---

## 모터 상태 전이 시나리오 (State Machine)

```text
[STOP] ----- (버튼 Short Press) ----> [CW]
[CW]   ----- (버튼 Short Press) ----> [SHORT_STOP] (200ms) -- (TIM4 만료) --> [CCW]
[CCW]  ----- (버튼 Short Press) ----> [SHORT_STOP] (200ms) -- (TIM4 만료) --> [CW]

[Any State] ----- (버튼 3초 Long Press) ----> [STOP] (TIM2 Timeout)
```

**3초 롱프레스 판단 메커니즘**
- 버튼 누름(Falling Edge) → TIM2(3초 One-shot 타이머) Start
- 3초 이내 버튼 뗌(Rising Edge) → TIM2 Stop, 방향 토글 수행
- 3초 초과 → TIM2 타임아웃 인터럽트 발생, 모터 강제 정지

---

## 파일 구조 및 역할

| 파일 | 기능 |
|---|---|
| `main.c` | 시스템 초기화, 이벤트 플래그 기반 메인 루프 |
| `gpio.c` | GPIO 초기화, CW/CCW/STOP 모드별 핀 설정 관리 |
| `key.c` | 버튼 입력용 EXTI 인터럽트 설정 |
| `timer.c` | TIM2(3초 타이머), TIM4(200ms 딜레이), TIM5(PWM) 설정 |
| `motor.c` | 모터 상태 전이 로직, PWM 방향 제어 |
| `uart.c` | USART2 초기화, 데이터 송수신 |
| `exception.c` | EXTI, TIM2, TIM4, USART2 인터럽트 핸들러 |

---

## 주요 트러블슈팅 및 문제 해결

### 1. 버튼 3초 롱프레스 판단 로직 (Polling → Interrupt-driven)

- **문제 (Stopwatch 방식, 인터럽트 미사용):** 다운카운터 타이머를 Stopwatch처럼 사용. 버튼 LOW → Start, 버튼 HIGH → Stop을 단순 `if` 체크로 구현했으나 시간이 제대로 측정되지 않아, `while`로 버튼이 HIGH가 될 때까지 대기한 뒤 Stop 시점의 CNT 값을 변수에 받아오도록 수정. CNT 값 자체는 하드웨어 카운터라 시간 측정은 정확했으나, **판단 시점이 "버튼을 뗀 순간"에 한정**되는 것이 근본 문제였음 — 버튼을 계속 누르고 있는 동안에는 3초가 지나도 정지 로직이 실행되지 않고, 반드시 버튼을 떼야만 모터가 정지함.
- **해결 (Interrupt-driven One-shot Timer):** TIM2를 3초 One-pulse 다운카운터로 구성하고, EXTI 버튼 Falling Edge에서 Start, Rising Edge에서 Stop 하도록 변경. 버튼을 계속 누르고 있어도 3초가 되는 순간 TIM2 자체의 타임아웃 인터럽트가 발생해 별도의 CNT 폴링 없이 3초를 초과하면 버튼 상태에 상관없이 즉시 정지 처리 가능.

### 2. TIM2 / TIM4 URS 비트 미설정으로 인한 중복·오작동 인터럽트

- **증상:** TIM2 — 버튼 두 번째 입력부터 타이머 재시작 직후 곧바로 인터럽트 발생, 모터 즉시 정지. TIM4 — Short Stop 딜레이 1회 실행에 인터럽트가 2회 발생.
- **원인:** `EGR->UG` Set(강제 Update Event)이 `CR1`의 URS(Update Request Source) 비트가 0인 상태에서는 실제 카운터 언더플로우가 아니어도 `UIF`를 함께 Set. 타이머 재시작/시작 시마다 EGR 갱신 자체가 인터럽트를 유발.

```c
// 원인 코드 (URS 누락)
Macro_Write_Block(TIM2->CR1, 0x3, 0x3, 3);  // down count, one pulse
```

- **해결:** `CR1`에 URS 비트 Set → 실제 카운터 언더플로우 시에만 `UIF` Set.

```c
// 수정 코드
Macro_Write_Block(TIM2->CR1, 0x7, 0x7, 2);  // down count, one pulse, URS
```

### 3. Active-Low 모터 드라이버 PWM Duty 불일치

- **증상:** 모터 드라이버 입력이 Active-Low라 Duty 20% 설정 시 실제 속도는 80%로 반전 매칭.
- **1차 해결:** 한쪽 출력(2Y) HIGH 고정, 반대쪽(1Y)에 PWM Mode 2(다운카운트 시 `CNT < CCR` 구간 Inactive) 적용해 Duty만큼 LOW 구간 생성.
- **추가 개선(REV 1.3):** 회로 구성 단순화를 위해 PWM을 실제 방향 핀(2Y)에 걸고 나머지 핀(1Y)은 GPIO Push-pull LOW 고정하는 구조로 변경, PWM 모드는 Mode 1로 환원. ODR 제어 로직도 `motor.c`에서 `gpio.c`의 `GPIO_Mode_CW`/`GPIO_Mode_CCW`로 이동해 방향 전환 시 GPIO/AF 설정을 한 곳에서 관리하도록 모듈화.

### 4. 방향 전환 시 논블로킹 Short Stop(200ms) 구현

- **문제:** 방향 급전환 시 필요한 200ms 정지 구간을, `delay()` 같은 블로킹 함수 없이 구현해야 함.
- **해결:** `Toggle_Motor_State()` 호출 시 모터가 이미 회전 중이면 즉시 반대 방향 구동 대신 목표 방향을 `next_state`에 예약, `Motor_Short_Stop()`으로 TIM4 200ms One-shot Start. TIM4 만료 인터럽트 발생 시 `Toggle_with_Short_Stop()`이 예약된 `next_state`를 읽어 실제 구동. 상태 예약과 실제 구동을 함수 단위로 분리해 메인 루프 멈춤 없이 지연 로직 구현.

### 5. 회로 결선 이슈

모터 무반응 현상의 원인이 코드가 아닌 DIP 핀 미체결 문제로 확인됨. 핀 맵 재확인 과정에서 소프트웨어 로직 외 실제 배선 상태 점검의 필요성 확인.

---

## REV 1.3 주요 변경 사항

- `timer.c` ARR 계산식에 `-1` 보정 추가 (의도한 카운트 값과 일치하도록 수정)
- Cortex-M4F FPU는 `double`(64bit) 미지원 → `float`(32bit) 전환으로 연산 오버헤드 제거
- PWM 출력 방식 변경: 핀 HIGH 고정 + 반대 핀 PWM(Mode 2) 방식 → PWM Mode 1 + 구동 핀 자체를 duty만큼 구동하는 방식
- ODR 설정 로직 `motor.c` → `gpio.c`(`GPIO_Mode_CW`/`GPIO_Mode_CCW`)로 이동, GPIO/AF 설정 일원화

---

## 향후 개선 사항

- 속도 테이블 범위를 벗어난 UART 입력에 대한 예외 처리/로그


[맨 위로 이동](#stm32f411-dc-motor-control-system-rev-13)