#include "heat_word_phrase.h"
#include "heat_word.h"

#include "esphome/components/light/addressable_light.h"
#include "esphome/core/color.h"

namespace esphome::heat_word {

// Logical word identifiers
enum WordId : uint8_t {
  WORD_MORE = 0,
  WORD_LESS,
  WORD_THAN,
  WORD_ONE,
  WORD_TWO,
  WORD_THREE,
  WORD_FOUR,
  WORD_FIVE_PLUS,
  WORD_SIX,
  WORD_SEVEN,
  WORD_EIGHT,
  WORD_NINE,
  WORD_TEN_HOUR,
  WORD_ELEVEN,
  WORD_HEAT,
  WORD_TWELVE,
  WORD_ERROR,
  WORD_TWENTY,
  WORD_TEN_MIN,
  WORD_THIRTY,
  WORD_FORTY,
  WORD_FIVE_TAIL,
  WORD_FIFTY,
  WORD_LEFT,
  WORD_DONE,
  WORD_COUNT,
};

// Pixel index tables for each word (logical indices 0..127).
static const uint8_t CELLS_MORE[] = {1, 2, 3, 4};
static const uint8_t CELLS_LESS[] = {6, 7, 8, 9};
static const uint8_t CELLS_THAN[] = {11, 12, 13, 14};
static const uint8_t CELLS_ONE[] = {16, 17, 18};
static const uint8_t CELLS_TWO[] = {19, 20, 21};
static const uint8_t CELLS_THREE[] = {22, 23, 24, 25, 26};
static const uint8_t CELLS_FOUR[] = {27, 28, 29, 30};
static const uint8_t CELLS_FIVE_PLUS[] = {33, 34, 35, 36};
static const uint8_t CELLS_SIX[] = {39, 40, 41};
static const uint8_t CELLS_SEVEN[] = {42, 43, 44, 45, 46};
static const uint8_t CELLS_EIGHT[] = {50, 51, 52, 53, 54};
static const uint8_t CELLS_NINE[] = {55, 56, 57, 58};
static const uint8_t CELLS_TEN_HOUR[] = {59, 60, 61};
static const uint8_t CELLS_TEN_MIN[] = {93, 94, 95};
static const uint8_t CELLS_ELEVEN[] = {64, 65, 66, 67, 68, 69};
static const uint8_t CELLS_HEAT[] = {70, 71, 72, 73};
static const uint8_t CELLS_TWELVE[] = {74, 75, 76, 77, 78, 79};
static const uint8_t CELLS_ERROR[] = {80, 81, 82, 83, 84};
static const uint8_t CELLS_TWENTY[] = {86, 87, 88, 89, 90, 91};
static const uint8_t CELLS_THIRTY[] = {96, 97, 98, 99, 100, 101};
static const uint8_t CELLS_FORTY[] = {102, 103, 104, 105, 106};
static const uint8_t CELLS_FIVE_TAIL[] = {108, 109, 110, 111};
static const uint8_t CELLS_FIFTY[] = {112, 113, 114, 115, 116};
static const uint8_t CELLS_LEFT[] = {118, 119, 120, 121};
static const uint8_t CELLS_DONE[] = {123, 124, 125, 126};

// Word definitions table — order must match WordId enum.
const Word WORDS[WORD_COUNT] = {
    /* WORD_MORE      */ {CELLS_MORE, sizeof(CELLS_MORE), WordKind::MISC, "MORE"},
    /* WORD_LESS      */ {CELLS_LESS, sizeof(CELLS_LESS), WordKind::MISC, "LESS"},
    /* WORD_THAN      */ {CELLS_THAN, sizeof(CELLS_THAN), WordKind::MISC, "THAN"},
    /* WORD_ONE       */ {CELLS_ONE, sizeof(CELLS_ONE), WordKind::HOUR, "ONE"},
    /* WORD_TWO       */ {CELLS_TWO, sizeof(CELLS_TWO), WordKind::HOUR, "TWO"},
    /* WORD_THREE     */ {CELLS_THREE, sizeof(CELLS_THREE), WordKind::HOUR, "THREE"},
    /* WORD_FOUR      */ {CELLS_FOUR, sizeof(CELLS_FOUR), WordKind::HOUR, "FOUR"},
    /* WORD_FIVE_PLUS */ {CELLS_FIVE_PLUS, sizeof(CELLS_FIVE_PLUS), WordKind::MINUTE, "FIVE"},
    /* WORD_SIX       */ {CELLS_SIX, sizeof(CELLS_SIX), WordKind::HOUR, "SIX"},
    /* WORD_SEVEN     */ {CELLS_SEVEN, sizeof(CELLS_SEVEN), WordKind::HOUR, "SEVEN"},
    /* WORD_EIGHT     */ {CELLS_EIGHT, sizeof(CELLS_EIGHT), WordKind::HOUR, "EIGHT"},
    /* WORD_NINE      */ {CELLS_NINE, sizeof(CELLS_NINE), WordKind::HOUR, "NINE"},
    /* WORD_TEN_HOUR  */ {CELLS_TEN_HOUR, sizeof(CELLS_TEN_HOUR), WordKind::HOUR, "TEN"},
    /* WORD_ELEVEN    */ {CELLS_ELEVEN, sizeof(CELLS_ELEVEN), WordKind::HOUR, "ELEVEN"},
    /* WORD_HEAT      */ {CELLS_HEAT, sizeof(CELLS_HEAT), WordKind::HEAT, "HEAT"},
    /* WORD_TWELVE    */ {CELLS_TWELVE, sizeof(CELLS_TWELVE), WordKind::HOUR, "TWELVE"},
    /* WORD_ERROR     */ {CELLS_ERROR, sizeof(CELLS_ERROR), WordKind::ERRORS, "ERROR"},
    /* WORD_TWENTY    */ {CELLS_TWENTY, sizeof(CELLS_TWENTY), WordKind::MINUTE, "TWENTY"},
    /* WORD_TEN_MIN   */ {CELLS_TEN_MIN, sizeof(CELLS_TEN_MIN), WordKind::MINUTE, "TEN"},
    /* WORD_THIRTY    */ {CELLS_THIRTY, sizeof(CELLS_THIRTY), WordKind::MINUTE, "THIRTY"},
    /* WORD_FORTY     */ {CELLS_FORTY, sizeof(CELLS_FORTY), WordKind::MINUTE, "FORTY"},
    /* WORD_FIVE_TAIL */ {CELLS_FIVE_TAIL, sizeof(CELLS_FIVE_TAIL), WordKind::MINUTE, "FIVE"},
    /* WORD_FIFTY     */ {CELLS_FIFTY, sizeof(CELLS_FIFTY), WordKind::MINUTE, "FIFTY"},
    /* WORD_LEFT      */ {CELLS_LEFT, sizeof(CELLS_LEFT), WordKind::MISC, "LEFT"},
    /* WORD_DONE      */ {CELLS_DONE, sizeof(CELLS_DONE), WordKind::DONE, "DONE"},
};
static_assert(std::size(WORDS) == WORD_COUNT, "WORDS table size mismatch");

void Word::render(HeatWordComponent *comp, light::AddressableLight *strip) const {
  const Color col = comp->word_color(this->category);
  for (uint8_t i = 0; i < this->len; i++) {
    (*strip)[this->cells[i]] = col;
  }
}

// --- Phrase building ---

// Hour → WordId lookup (index 0 = 1 hour, index 11 = 12 hours).
static constexpr WordId HOUR_WORDS[12] = {
    WORD_ONE,   WORD_TWO,   WORD_THREE, WORD_FOUR,     WORD_FIVE_PLUS, WORD_SIX,
    WORD_SEVEN, WORD_EIGHT, WORD_NINE,  WORD_TEN_HOUR, WORD_ELEVEN,    WORD_TWELVE,
};

static WordId hour_word(uint8_t h) {
  if (h < 1 || h > 12)
    return WORD_TWELVE;
  return HOUR_WORDS[h - 1];
}

// Indexed by snapped / 5: index 0 = 0 min (nothing), 1 = 5 min, ..., 11 = 55 min.
static constexpr Phrase MINUTE_MASKS[12] = {
    0,                                              // 0 min
    (1u << WORD_FIVE_PLUS),                         // 5 min
    (1u << WORD_TEN_MIN),                           // 10 min
    (1u << WORD_TEN_MIN) | (1u << WORD_FIVE_PLUS),  // 15 min
    (1u << WORD_TWENTY),                            // 20 min
    (1u << WORD_TWENTY) | (1u << WORD_FIVE_PLUS),   // 25 min
    (1u << WORD_THIRTY),                            // 30 min
    (1u << WORD_THIRTY) | (1u << WORD_FIVE_PLUS),   // 35 min
    (1u << WORD_FORTY),                             // 40 min
    (1u << WORD_FORTY) | (1u << WORD_FIVE_TAIL),    // 45 min
    (1u << WORD_FIFTY),                             // 50 min
    (1u << WORD_FIFTY) | (1u << WORD_FIVE_PLUS),    // 55 min
};

static void mark_minute_words(Phrase &phrase, uint8_t snapped) {
  const uint8_t idx = snapped / 5;
  if (idx < std::size(MINUTE_MASKS))
    phrase |= MINUTE_MASKS[idx];
}

static void mark_duration_words(Phrase &phrase, uint32_t seconds) {
  uint32_t h = seconds / 3600;
  const uint32_t m = (seconds % 3600) / 60;
  uint8_t snapped = static_cast<uint8_t>(((m + 2) / 5) * 5);
  if (snapped >= 60) {
    snapped = 0;
    h++;
  }
  if (h >= 1) {
    const uint8_t hh = static_cast<uint8_t>(h > 12 ? 12 : h);
    phrase |= (1u << hour_word(hh));
  }
  mark_minute_words(phrase, snapped);
}

Phrase build_phrase(PrintState state, uint32_t remaining_seconds, uint32_t total_seconds) {
  Phrase phrase = 0;
  switch (state) {
    case PrintState::IDLE:
      break;
    case PrintState::HEATING:
      phrase |= (1u << WORD_HEAT);
      if (remaining_seconds > 0)
        mark_duration_words(phrase, remaining_seconds);
      break;
    case PrintState::PRINTING:
      if (total_seconds > 0) {
        const uint64_t twice = static_cast<uint64_t>(remaining_seconds) * 2;
        if (twice > total_seconds) {
          phrase |= (1u << WORD_MORE) | (1u << WORD_THAN);
        } else if (twice < total_seconds && remaining_seconds > 0) {
          phrase |= (1u << WORD_LESS) | (1u << WORD_THAN);
        }
      }
      if (remaining_seconds > 0)
        mark_duration_words(phrase, remaining_seconds);
      break;
    case PrintState::PAUSED:
      phrase |= (1u << WORD_LEFT);
      if (remaining_seconds > 0)
        mark_duration_words(phrase, remaining_seconds);
      break;
    case PrintState::ERROR:
      phrase |= (1u << WORD_ERROR);
      break;
    case PrintState::DONE:
      phrase |= (1u << WORD_DONE);
      break;
  }
  return phrase;
}

void render_phrase(Phrase phrase, HeatWordComponent *comp, light::AddressableLight *strip) {
  for (int32_t i = 0; i < STRIP_SIZE; i++)
    (*strip)[i] = comp->word_color(WordKind::BACKGROUND);

  for (uint8_t w = 0; w < WORD_COUNT; w++) {
    if (phrase & (1u << w))
      WORDS[w].render(comp, strip);
  }
}

}  // namespace esphome::heat_word
