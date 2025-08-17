# Projeto Adaptador HDMI

Este projeto descreve a pinagem e documentação para conexão de um microcontrolador ao conector HDMI, visando facilitar o desenvolvimento e montagem do adaptador.

## Como obter este módulo (via workspace principal)

Para garantir todas as dependências (ex.: drivers comuns e bibliotecas em outros diretórios do workspace), recomenda-se clonar o repositório raiz (monorepo) com submódulos:

```bash
git clone --recurse-submodules https://github.com/RapportTecnologia/Sensores_e_Atuadores_do_Kit_BitDogLab.git Sensores_e_Atuadores_do_KIT
cd Sensores_e_Atuadores_do_KIT
git submodule update --init --recursive
```

Para atualizar submódulos para suas últimas revisões (quando configurado):

```bash
git submodule update --remote --merge --recursive
```

Caso esteja configurando do zero no workspace raiz, inicialize/atualize submódulos antes de compilar:

```bash
cmake -S . -B build
cmake --build build -j
```

## Pinagem Utilizada

| Nº | Pino     | Função/Descrição   |
|----|----------|--------------------|
|  1 | GND      | Referência         |
|  2 | VSYS     | 5V                 |
|  3 | 3.3V     | 3.3V               |
|  4 | GP8      | GPIO               |
|  5 | GP28     | GPIO               |
|  6 | GP9      | GPIO               |
|  7 | AGND     | Terra Analógico    |
|  8 | GP4      | GPIO               |
|  9 | GP17     | D1 HDMI            |
| 10 | GP20     | GPIO               |
| 11 | GP16     | D0 HDMI            |
| 12 | GP19     | Clk HDMI           |
| 13 | GND      | Referência         |
| 14 | GP18     | D2 HDMI            |

> **Observação:** Certifique-se de conectar corretamente os pinos de referência (GND e AGND) para evitar ruídos e garantir o funcionamento adequado do HDMI.

## Chip de Sinal Diferencial

O projeto utiliza o chip **DS90LV027A** para gerar o sinal diferencial necessário para a transmissão HDMI. Este CI é responsável por converter sinais digitais simples em sinais diferenciais, fundamentais para a integridade e qualidade do sinal HDMI.

### Função do DS90LV027A
- Conversão de sinais digitais (TTL/CMOS) em sinais diferenciais (LVDS)
- Redução de ruído e interferência eletromagnética
- Melhora a integridade do sinal em altas velocidades

## Botões da BitDogLab

Botão A Pino 5
Botão B Pino 6

## Objetivo

Documentar e padronizar a conexão dos pinos para facilitar a montagem e o desenvolvimento de aplicações com HDMI neste kit.

---

Se precisar adicionar mais detalhes, diagramas ou instruções, é só pedir!

## Dependência OLED

Este projeto utiliza o driver OLED do repositório externo clonado como submódulo no raiz do workspace:

- Diretório: `../OLED_SSD1306/`
- Repositório: `git@github.com:RapportTecnologia/OLED_SSD1306-RP2040-RP2350.git`

Os arquivos fonte `oled.c` e `ssd1306_i2c.c` são referenciados diretamente do submódulo pelo `CMakeLists.txt` deste projeto. Os cabeçalhos (`oled.h`, `ssd1306_i2c.h`, etc.) também são incluídos a partir desse diretório.

## Como compilar

Para evitar conflitos com caches antigos, gere um diretório de build dedicado:

```bash
cmake -S . -B build
cmake --build build -j
```

Caso esteja configurando do zero no workspace raiz, inicialize/atualize submódulos antes de compilar:

```bash
git submodule update --init --recursive
```
