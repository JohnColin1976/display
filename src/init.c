#include "sam3xa.h"
#include "init.h"

/* *****************************************************************
    INIT BLOCK
***************************************************************** */

void uart_init(void) {
    /* Включить тактирование PIOA + USART0 */
    PMC->PMC_WPMR = 0x504D4300u;
    PMC->PMC_PCER0 = (1u << ID_PIOA) | (1u << ID_USART0);

    /* 2. Настроить PA10 (RXD0) и PA11 (TXD0) как периферию A */
    PIOA->PIO_PDR   = (1u << 10) | (1u << 11); // отключить PIO
    PIOA->PIO_ABSR &= ~((1u << 10) | (1u << 11)); // Peripheral A

    /* 3. Сброс и отключение USART */
    USART0->US_CR = US_CR_RSTRX | US_CR_RSTTX |
                    US_CR_RXDIS | US_CR_TXDIS;

    /* 4. Режим: асинхронный, 8N1, без паритета */
    USART0->US_MR =
        US_MR_USART_MODE_NORMAL |
        US_MR_USCLKS_MCK |
        US_MR_CHRL_8_BIT |
        US_MR_PAR_NO |
        US_MR_NBSTOP_1_BIT;

    /* 5) Baud = MCK / (16 * (CD + FP/8))
       MCK=84MHz, target 115200 -> CD=45, FP=5 (~115068, error about -0.11%) */
    USART0->US_BRGR = US_BRGR_CD(45u) | US_BRGR_FP(5u);

    /* 6. Включить приём и передачу */
    USART0->US_CR = US_CR_RXEN | US_CR_TXEN;
}

void uart1_init(void)
{
    /* Включить тактирование PIOA + USART1 */
    PMC->PMC_WPMR = 0x504D4300u;
    PMC->PMC_PCER0 = (1u << ID_PIOA) | (1u << ID_USART1);

    /* 2) PA12(TXD1) + PA13(RXD1) -> Peripheral A */
    PIOA->PIO_PDR   = (1u << 12) | (1u << 13);   // отключить PIO, отдать периферии
    PIOA->PIO_ABSR &= ~((1u << 12) | (1u << 13));/* 0 = Peripheral A */

    /* (опционально) подтяжка на RX */
    PIOA->PIO_PUER  = (1u << 13);

    /* 3) Сброс и отключение TX/RX */
    USART1->US_CR = US_CR_RSTRX | US_CR_RSTTX |
                    US_CR_RXDIS | US_CR_TXDIS;

    /* 4) Режим: async, MCK, 8N1 */
    USART1->US_MR =
        US_MR_USART_MODE_NORMAL |
        US_MR_USCLKS_MCK |
        US_MR_CHRL_8_BIT |
        US_MR_PAR_NO |
        US_MR_NBSTOP_1_BIT;

    /* 5) Baud = MCK / (16 * (CD + FP/8))
       MCK=84MHz, target 115200 -> CD=45, FP=5 (~115068, error about -0.11%) */
    USART1->US_BRGR = US_BRGR_CD(45u) | US_BRGR_FP(5u);

    /* 6) Включить TX/RX */
    USART1->US_CR = US_CR_RXEN | US_CR_TXEN;
}

static inline void uart_putc(char c)
{
  while ((USART0->US_CSR & US_CSR_TXRDY) == 0u) {}
  USART0->US_THR = (uint32_t)c;
}

static void uart_puts(const char *s)
{
  while (*s != '\0') {
    uart_putc(*s++);
  }
}

// Инициализация PB27
void gpio_init_out(void) {
  TEST_PIO->PIO_PER  = TEST_MASK;
  TEST_PIO->PIO_OER  = TEST_MASK;
  TEST_PIO->PIO_CODR = TEST_MASK;
}

// Инициализация PB26 для вывода сигнала синхронизации
void sync_out_init(void) {
  PMC->PMC_PCER0 = (1u << ID_PIOB);
  SYNC_OUT_PIO->PIO_PER = SYNC_OUT_MASK;
  SYNC_OUT_PIO->PIO_OER = SYNC_OUT_MASK;
  SYNC_OUT_PIO->PIO_CODR = SYNC_OUT_MASK; // low
}

void dacc_init(void) {
  // Включить тактирование DACC (ID_DACC в PMC_PCER1, т.к. >31)
  PMC->PMC_PCER1 = (1u << (ID_DACC - 32));

  // (Опционально) снять защиту записи DACC, если включена
  // В SAM3X у DACC есть WPMR. Для начала можно просто отключить WP:
  DACC->DACC_WPMR = 0x44414300; // "DAC", WPEN=0

  // Сброс
  DACC->DACC_CR = DACC_CR_SWRST;

  // Режим:
  // - TRGEN_DIS: обновляем вручную
  // - WORD_HALF: 12-bit
  // - TAG_EN: удобно писать в один регистр и выбирать канал
  DACC->DACC_MR =
      DACC_MR_TRGEN_DIS |
      DACC_MR_WORD_HALF |
      DACC_MR_TAG_EN |
      DACC_MR_STARTUP_8;

  // Разрешить каналы
  DACC->DACC_CHER = DACC_CHER_CH0 | DACC_CHER_CH1;
}

void spi0_init(void)
{
  /* Enable clocks for SPI0 and PIOA (SPI0 pins are on port A). */
  PMC->PMC_PCER0 = (1u << ID_SPI0) | (1u << ID_PIOA);

  /* PA25=MOSI0, PA26=MISO0, PA27=SPCK0, PA28=NPCS0 -> Peripheral A. */
  PIOA->PIO_PDR = (1u << 25) | (1u << 26) | (1u << 27) | (1u << 28);
  PIOA->PIO_ABSR &= ~((1u << 25) | (1u << 26) | (1u << 27) | (1u << 28));

  /* Reset and configure SPI0 as master. */
  SPI0->SPI_CR = SPI_CR_SWRST;
  SPI0->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS | SPI_MR_PCS(0xEu);

  /* CS0: 8-bit transfers, conservative startup baudrate (MCK/21 ~ 4 MHz). */
  SPI0->SPI_CSR[0] = SPI_CSR_BITS_8_BIT | SPI_CSR_SCBR(21u);

  SPI0->SPI_CR = SPI_CR_SPIEN;
}

// ===== CONFIG =====
#define TFT_W 480
#define TFT_H 320

#define SPI_SCBR_DIV  32u
#define SPI_MODE      0u
#define MADCTL_VALUE  0x28u
#define COLMOD_VALUE  0x55u
// ==================

// D6=PC24 DC, D5=PC25 RST, D10 CS (PA28 + PC29) -> drive both
#define DC_PIO   PIOC
#define DC_BIT   (1u << 24)
#define RST_PIO  PIOC
#define RST_BIT  (1u << 25)
#define CS_PIOA  PIOA
#define CS_A_BIT (1u << 28)
#define CS_PIOC  PIOC
#define CS_C_BIT (1u << 29)

// SPI0 Peripheral A pins (SPI header Due)
#define SPI_MISO (1u << 25)
#define SPI_MOSI (1u << 26)
#define SPI_SCK  (1u << 27)

static inline void delay_cycles(volatile uint32_t c) { while (c--) { __NOP(); } }
static void delay_ms(uint32_t ms) { while (ms--) { delay_cycles(84000u / 4u); } }

static inline void dc_low(void)  { DC_PIO->PIO_CODR = DC_BIT; }
static inline void dc_high(void) { DC_PIO->PIO_SODR = DC_BIT; }
static inline void rst_low(void) { RST_PIO->PIO_CODR = RST_BIT; }
static inline void rst_high(void){ RST_PIO->PIO_SODR = RST_BIT; }
static inline void cs_low(void)  { CS_PIOA->PIO_CODR = CS_A_BIT; CS_PIOC->PIO_CODR = CS_C_BIT; }
static inline void cs_high(void) { CS_PIOA->PIO_SODR = CS_A_BIT; CS_PIOC->PIO_SODR = CS_C_BIT; }

static inline void spi0_write8(uint8_t v)
{
  while ((SPI0->SPI_SR & SPI_SR_TDRE) == 0u) {}
  SPI0->SPI_TDR = (uint32_t)v;
  while ((SPI0->SPI_SR & SPI_SR_RDRF) == 0u) {}
  (void)SPI0->SPI_RDR;
}

// 16-bit frame write
static inline void spi0_write16(uint16_t v)
{
  while ((SPI0->SPI_SR & SPI_SR_TDRE) == 0u) {}
  SPI0->SPI_TDR = (uint32_t)v;
  while ((SPI0->SPI_SR & SPI_SR_RDRF) == 0u) {}
  (void)SPI0->SPI_RDR;
}

static inline void tft_begin(void) { cs_low(); }
static inline void tft_end(void)   { cs_high(); }
static inline void tft_cmd(uint8_t c)   { dc_low(); spi0_write8(c); }
static inline void tft_data8(uint8_t d) { dc_high(); spi0_write8(d); }

static void spi0_init_16bit(void)
{
  PMC->PMC_WPMR = 0x504D4300u;
  PMC->PMC_PCER0 = (1u << ID_PIOA) | (1u << ID_PIOC) | (1u << ID_SPI0);

  // GPIO
  PIOC->PIO_PER  = DC_BIT | RST_BIT | CS_C_BIT;
  PIOC->PIO_OER  = DC_BIT | RST_BIT | CS_C_BIT;
  PIOC->PIO_SODR = DC_BIT | RST_BIT | CS_C_BIT;

  PIOA->PIO_PER  = CS_A_BIT;
  PIOA->PIO_OER  = CS_A_BIT;
  PIOA->PIO_SODR = CS_A_BIT;

  // SPI pins -> Peripheral A
  uint32_t perA = SPI_MISO | SPI_MOSI | SPI_SCK;
  PIOA->PIO_PDR  = perA;
  PIOA->PIO_ABSR &= ~perA;

  // Reset SPI
  SPI0->SPI_CR = SPI_CR_SWRST;
  SPI0->SPI_CR = SPI_CR_SWRST;

  // Master, disable mode fault. CS is controlled as GPIO.
  SPI0->SPI_MR = SPI_MR_MSTR | SPI_MR_MODFDIS;

  // CSR: 16-bit frames
  uint32_t csr = SPI_CSR_BITS_16_BIT | SPI_CSR_SCBR(SPI_SCBR_DIV);

  // MODE0: CPOL=0 NCPHA=1 ; MODE1: CPOL=0 NCPHA=0 ; MODE2: CPOL=1 NCPHA=1 ; MODE3: CPOL=1 NCPHA=0
  switch (SPI_MODE) {
    case 0u: csr |= SPI_CSR_NCPHA; break;
    case 1u: break;
    case 2u: csr |= SPI_CSR_CPOL | SPI_CSR_NCPHA; break;
    case 3u: csr |= SPI_CSR_CPOL; break;
    default: csr |= SPI_CSR_NCPHA; break;
  }

  SPI0->SPI_CSR[0] = csr;
  SPI0->SPI_CR = SPI_CR_SPIEN;

  cs_high();
  dc_high();
  rst_high();
}

static void ili9486_init_like_lib(void)
{
  rst_high(); delay_ms(5u);
  rst_low();  delay_ms(20u);
  rst_high(); delay_ms(150u);

  tft_begin(); tft_cmd(0x01u); tft_end(); delay_ms(150u); // SWRESET
  tft_begin(); tft_cmd(0x11u); tft_end(); delay_ms(180u); // SLPOUT

  tft_begin(); tft_cmd(0x36u); tft_data8(MADCTL_VALUE); tft_end();
  tft_begin(); tft_cmd(0x3Au); tft_data8(COLMOD_VALUE); tft_end();

  tft_begin(); tft_cmd(0x13u); tft_end(); delay_ms(10u);  // Normal display mode on
  tft_begin(); tft_cmd(0x29u); tft_end(); delay_ms(50u);  // Display on
}

static inline void window_ramwr_begin(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  tft_begin();
  tft_cmd(0x2Au);
  dc_high();
  spi0_write8((uint8_t)(x0 >> 8)); spi0_write8((uint8_t)x0);
  spi0_write8((uint8_t)(x1 >> 8)); spi0_write8((uint8_t)x1);

  dc_low(); spi0_write8(0x2Bu);
  dc_high();
  spi0_write8((uint8_t)(y0 >> 8)); spi0_write8((uint8_t)y0);
  spi0_write8((uint8_t)(y1 >> 8)); spi0_write8((uint8_t)y1);

  dc_low(); spi0_write8(0x2Cu);
  dc_high();
}

static inline uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint16_t)(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
}

// Same as common Arduino libs for ILI9486 over SPI.
static inline uint16_t swap16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }

static void fill_screen(uint16_t c565)
{
  window_ramwr_begin(0u, 0u, TFT_W - 1u, TFT_H - 1u);

  uint16_t w = swap16(c565);
  for (uint32_t i = 0u; i < ((uint32_t)TFT_W * (uint32_t)TFT_H); i++) {
    spi0_write16(w);
  }
  tft_end();
}

static void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c565)
{
  if (x >= TFT_W || y >= TFT_H) { return; }
  if ((uint32_t)x + (uint32_t)w > TFT_W) { w = (uint16_t)(TFT_W - x); }
  if ((uint32_t)y + (uint32_t)h > TFT_H) { h = (uint16_t)(TFT_H - y); }

  window_ramwr_begin(x, y, (uint16_t)(x + w - 1u), (uint16_t)(y + h - 1u));

  uint16_t ww = swap16(c565);
  for (uint32_t i = 0u; i < ((uint32_t)w * (uint32_t)h); i++) {
    spi0_write16(ww);
  }
  tft_end();
}

static const uint8_t font5x7_digits[10][5] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
  {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
  {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
  {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}
};

static const uint8_t font5x7_letters_af[6][5] = {
  {0x7E, 0x11, 0x11, 0x11, 0x7E}, // A
  {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
  {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7F, 0x09, 0x09, 0x09, 0x01}  // F
};

static const uint8_t font5x7_letters_gz[20][5] = {
  {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
  {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
  {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
  {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
  {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
  {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
  {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
  {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
  {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
  {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
  {0x7F, 0x20, 0x18, 0x20, 0x7F}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x03, 0x04, 0x78, 0x04, 0x03}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}  // Z
};

static const uint8_t glyph_space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t glyph_dot[5]   = {0x00, 0x60, 0x60, 0x00, 0x00};
static const uint8_t glyph_dash[5]  = {0x08, 0x08, 0x08, 0x08, 0x08};
static const uint8_t glyph_colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
static const uint8_t glyph_slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
static const uint8_t glyph_plus[5]  = {0x08, 0x08, 0x3E, 0x08, 0x08};
static const uint8_t glyph_under[5] = {0x40, 0x40, 0x40, 0x40, 0x40};

static volatile uint32_t g_ms_ticks = 0u;

void SysTick_Handler(void)
{
  g_ms_ticks++;
}

static inline uint32_t millis(void)
{
  return g_ms_ticks;
}

static const uint8_t *glyph5x7(char c)
{
  if (c >= '0' && c <= '9') { return font5x7_digits[(uint8_t)(c - '0')]; }
  if (c >= 'a' && c <= 'z') { c = (char)(c - ('a' - 'A')); }
  if (c >= 'A' && c <= 'F') { return font5x7_letters_af[(uint8_t)(c - 'A')]; }
  if (c >= 'G' && c <= 'Z') { return font5x7_letters_gz[(uint8_t)(c - 'G')]; }
  if (c == ' ') { return glyph_space; }
  if (c == '.') { return glyph_dot; }
  if (c == '-') { return glyph_dash; }
  if (c == ':') { return glyph_colon; }
  if (c == '/') { return glyph_slash; }
  if (c == '+') { return glyph_plus; }
  if (c == '_') { return glyph_under; }
  return 0;
}

static void draw_char(uint16_t x, uint16_t y, char c, uint8_t s, uint16_t fg, uint16_t bg)
{
  const uint8_t *cols = glyph5x7(c);

  fill_rect(x, y, (uint16_t)(5u * s), (uint16_t)(7u * s), bg);
  if (cols == 0) { return; }

  for (uint8_t cx = 0u; cx < 5u; cx++) {
    uint8_t bits = cols[cx];
    for (uint8_t cy = 0u; cy < 7u; cy++) {
      if ((bits & (1u << cy)) != 0u) {
        fill_rect((uint16_t)(x + cx * s), (uint16_t)(y + cy * s), s, s, fg);
      }
    }
  }
}

static uint16_t text_width_px(const char *p, uint8_t s)
{
  uint16_t w = 0u;
  while (*p != '\0') { w = (uint16_t)(w + 6u * s); p++; }
  if (w != 0u) { w = (uint16_t)(w - s); }
  return w;
}

static void draw_text_center_y(const char *p, uint8_t s, uint16_t y, uint16_t fg, uint16_t bg)
{
  uint16_t tw = text_width_px(p, s);
  uint16_t x = (tw < TFT_W) ? (uint16_t)((TFT_W - tw) / 2u) : 0u;
  while (*p != '\0') {
    draw_char(x, y, *p, s, fg, bg);
    x = (uint16_t)(x + 6u * s);
    p++;
  }
}

static uint32_t u32_to_dec(char *out, uint32_t max_len, uint32_t v)
{
  char tmp[10];
  uint32_t n = 0u;
  uint32_t pos = 0u;

  if (max_len == 0u) { return 0u; }

  if (v == 0u) {
    if (max_len > 1u) {
      out[0] = '0';
      out[1] = '\0';
      return 1u;
    }
    out[0] = '\0';
    return 0u;
  }

  while (v != 0u && n < (uint32_t)sizeof(tmp)) {
    uint32_t q = v / 10u;
    tmp[n++] = (char)('0' + (v - q * 10u));
    v = q;
  }

  while (n != 0u && (pos + 1u) < max_len) {
    out[pos++] = tmp[--n];
  }
  out[pos] = '\0';
  return pos;
}

static uint32_t bytes_to_ascii(char *out, uint32_t max_len, const uint8_t *data, uint32_t len)
{
  uint32_t pos = 0u;

  if (max_len == 0u) { return 0u; }

  for (uint32_t i = 0u; i < len; i++) {
    char c;
    if ((pos + 1u) >= max_len) { break; }
    c = (char)data[i];
    if (c >= 'a' && c <= 'z') { c = (char)(c - ('a' - 'A')); }
    if (c < 32 || c > 126) { c = '.'; }
    out[pos++] = c;
  }

  out[pos] = '\0';
  return pos;
}

static inline int uart_try_read(uint8_t *b)
{
  if ((USART0->US_CSR & US_CSR_RXRDY) == 0u) { return 0; }
  *b = (uint8_t)(USART0->US_RHR & 0xFFu);
  return 1;
}

static void show_rx(uint32_t t_ms, const uint8_t *data, uint32_t len)
{
  char line_time[12];
  char line_data[41];

  if (len > 40u) { len = 40u; }

  u32_to_dec(line_time, (uint32_t)sizeof(line_time), t_ms);
  bytes_to_ascii(line_data, (uint32_t)sizeof(line_data), data, len);

  fill_rect(0u, 60u, TFT_W, 40u, RGB565(0u, 0u, 0u));
  fill_rect(0u, 140u, TFT_W, 32u, RGB565(0u, 0u, 0u));
  draw_text_center_y(line_time, 3u, 70u, RGB565(0u, 255u, 0u), RGB565(0u, 0u, 0u));
  draw_text_center_y(line_data, 2u, 150u, RGB565(255u, 255u, 255u), RGB565(0u, 0u, 0u));
}

void setup(void)
{
  spi0_init_16bit();
  ili9486_init_like_lib();
  uart_init();
  uart_puts("Display loaded.\r\n");

  SystemCoreClockUpdate();
  (void)SysTick_Config(SystemCoreClock / 1000u);

  fill_screen(RGB565(0u, 0u, 0u));
}

void loop(void)
{
  static uint8_t rx_buf[64];
  static uint32_t rx_len = 0u;
  static uint32_t last_rx_ms = 0u;
  static uint32_t last_tx_ms = 0u;
  uint8_t b = 0u;
  uint32_t now = millis();

  if ((now - last_tx_ms) >= 1000u) {
    char tbuf[12];
    last_tx_ms = now;
    u32_to_dec(tbuf, (uint32_t)sizeof(tbuf), now);
    uart_puts(tbuf);
    uart_puts("\r\n");
  }

  while (uart_try_read(&b) != 0) {
    last_rx_ms = millis();
    if (b == '\r' || b == '\n') {
      if (rx_len != 0u) {
        show_rx(last_rx_ms, rx_buf, rx_len);
        rx_len = 0u;
      }
      continue;
    }

    if (rx_len < (uint32_t)sizeof(rx_buf)) {
      rx_buf[rx_len++] = b;
    } else {
      show_rx(last_rx_ms, rx_buf, rx_len);
      rx_len = 0u;
    }
  }

  // Show partial frame if no newline came.
  if (rx_len != 0u) {
    now = millis();
    if ((now - last_rx_ms) >= 40u) {
      show_rx(now, rx_buf, rx_len);
      rx_len = 0u;
    }
  }
}
