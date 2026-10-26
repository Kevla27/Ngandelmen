# Resolve addresses from Drivers/CMSIS/Device/ST/STM32U5xx/Include/stm32u575xx.h
import re

macros = {}
with open('Drivers/CMSIS/Device/ST/STM32U5xx/Include/stm32u575xx.h', 'r') as f:
    for line in f:
        line = line.split('/*')[0].split('//')[0].strip()
        m = re.match(r'#define\s+([A-Za-z0-9_]+)\s+(.+)', line)
        if m:
            name = m.group(1).strip()
            val = m.group(2).strip()
            macros[name] = val

def eval_macro(name, depth=0):
    if depth > 20: return 0
    if name not in macros:
        clean = name.replace('UL', '').replace('U', '').replace('L', '').strip()
        try:
            return int(clean, 0)
        except:
            return 0
    raw = macros[name]
    # replace tokens
    tokens = re.split(r'([+\-*/ ()])', raw)
    new_tokens = []
    for t in tokens:
        st = t.strip()
        if st in macros:
            new_tokens.append(str(eval_macro(st, depth+1)))
        else:
            new_tokens.append(t.replace('UL', '').replace('U', '').replace('L', ''))
    expr = ''.join(new_tokens)
    try:
        return eval(expr)
    except:
        return 0

targets = [
    'GPIOA_BASE_NS', 'GPIOA_BASE_S',
    'GPIOB_BASE_NS', 'GPIOB_BASE_S',
    'GPIOC_BASE_NS', 'GPIOC_BASE_S',
    'GPIOD_BASE_NS', 'GPIOD_BASE_S',
    'GPIOE_BASE_NS', 'GPIOE_BASE_S',
    'GPIOH_BASE_NS', 'GPIOH_BASE_S',
    'LPUART1_BASE_NS', 'LPUART1_BASE_S',
    'USART3_BASE_NS', 'USART3_BASE_S',
    'SPI1_BASE_NS', 'SPI1_BASE_S',
    'SPI2_BASE_NS', 'SPI2_BASE_S',
    'I2C3_BASE_NS', 'I2C3_BASE_S',
    'ADC4_BASE_NS', 'ADC4_BASE_S',
    'RTC_BASE_NS', 'RTC_BASE_S',
    'TAMP_BASE_NS', 'TAMP_BASE_S',
    'PWR_BASE_NS', 'PWR_BASE_S',
    'EXTI_BASE_NS', 'EXTI_BASE_S',
    'RCC_BASE_NS', 'RCC_BASE_S'
]

for t in targets:
    v = eval_macro(t)
    print(f"{t}: 0x{v:08X}")

