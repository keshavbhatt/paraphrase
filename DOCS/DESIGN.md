# DESIGN: Paraphrase

Principle: **select, pick, done**. Select text in any app, press the shortcut, pick an
action, and the result goes back in place; nothing else competes for attention, and the
user can always see what changed and that nothing left the computer.
Every screen has an HTML mock in `mocks/` (open `mocks/index.html`; the toolbar at the bottom
right switches dark and light, the OS title bar, and the review notes). No screen or visible
change is coded before its mock is approved; approval is recorded per screen in
`mocks/index.html` and `PROGRESS.md` (CLAUDE.md standing rule). PLAN 10: no UI code before
`DOCS/models.md` has the M0 go/no-go.

## 1. Brand and tokens

- Name: Paraphrase. Mark: a pen (placeholder until an app icon exists).
- One accent, an ink blue, for the main action, focus and links. Green only says "private"
  and "done"; orange and blue are kept for the diff, so a highlighted change is never
  mistaken for a button. `mocks/mock.css` is the source of these tokens until the app's
  style code exists; the two must never disagree.

| Token | Dark | Light | Used for |
|---|---|---|---|
| `accent` | `#9DB0FF` | `#2747C9` | focus ring, links, selected reply border, text on accent-soft |
| `accentStrong` | `#3B5BDB` | `#2747C9` | primary button fill (white text), selected tone, highlighted action |
| `accentHover` | `#4C6CEB` | `#1B3399` | hover on primary |
| `accentSoft` | `#222C52` | `#E3EAFF` | selected reply, recommended pill, current settings section |
| `bg` | `#17181A` | `#F4F4F0` | window background |
| `bar` | `#1E1F22` | `#FFFFFF` | popup footer, setup header and footer, title bar |
| `panel` | `#202124` | `#FFFFFF` | popup, cards |
| `elevated` | `#26272B` | `#FFFFFF` | menus, toasts, review toolbar |
| `sunk` | `#2C2D31` | `#F3F3EE` | the captured text and the user's original |
| `hover` | `#313237` | `#EEEEE8` | hover, neutral pills, number badges |
| `input` | `#151618` | `#FFFFFF` | text fields |
| `border` | `#393B41` | `#DCDCD5` | hairlines |
| `text` | `#ECEDEF` | `#1C1D1F` | primary text |
| `muted` | `#A3A6AD` | `#5C5F66` | labels, secondary text |
| `ins` / `insBg` | `#B4C2FF` / `#26336A` | `#1E3FA8` / `#E3EAFF` | inserted words in the diff (underlined) |
| `del` / `delBg` | `#FDBA8C` / `#4A2716` | `#9A3412` / `#FDEBDD` | deleted words in the diff (struck out) |
| `private` | `#82C99E` | `#3D6B4F` | "On this device", the privacy lines |
| `success` / `warning` / `danger` | `#82C99E` / `#F5B14C` / `#FF8A80` | `#2F6B45` / `#B45309` / `#B42318` | states |

Type: the system UI font (Qt uses the desktop's), with Noto fallbacks for Devanagari, Tamil,
Bengali and the other scripts users write in. 12 px labels, 14 px UI, 15 px text the user
reads or changes. Shape: 8 px controls, 10 px chips and result boxes, 12 px cards, 14 px
popup.

## 2. Layout

```
+--------------------------------------------+
| [pen] Paraphrase  from Telegram   [lock] x |  header: source app, privacy, close
+--------------------------------------------+
| SELECTED TEXT  16 words, English           |
| [ Mia dont know where the keys are, ...  ] |  captured text (sunk)
| ACTIONS                                    |
| [1 Proofread] [2 Tone v] [3 Reply] ...     |  numbered skills, the likely one highlighted
+--------------------------------------------+
| * Reading the text for Proofread...  Esc   |  status in words, never a percentage
+--------------------------------------------+
```

The popup is about 440 px wide, opens near the cursor and never steals focus longer than
needed. After an action the same panel shows the result with Back to Actions; the footer
holds the main action (Replace or Insert, Enter) and Copy and Retry.

## 3. Screens

| Mock | Shows | Milestone |
|---|---|---|
| `main-window.html` | Standalone: your text, every action, the diff, Copy, the missing-integration notice (ADR-013) | M2 |
| `main-window-reply.html` | Standalone reply: message, optional draft, three replies to copy | M2 |
| `main-window-busy.html` | While a run goes: the working pill (step in words, seconds so far), the shape of the answer until its first word, the caret while it streams | M2 |
| `menus.html` | Tone, Format, Translate and My actions menus with an icon per item; languages in their own script | M2 |
| `popup-actions.html` | Captured text, numbered actions, reading ahead | M2 |
| `popup-proofread.html` | Word diff, one line per change with Undo, Replace | M2 |
| `popup-reply.html` | Three replies in the message's language, edit, Insert | M2 |
| `popup-rewrite.html` | Tone presets, streaming result, versions, facts and language kept | M2 |
| `setup-model.html` | First run: hardware probe, fast and quality models, download source and check | M3 |
| `settings-models.html` | Settings: tier defaults, installed models, speed-up, languages | M3 |

## 4. Rules that survive the mocks

- Standalone first (ADR-013): every action works in the main window with no desktop
  integration; a missing integration shows a notice that says what to do instead, never an
  error, and the popup can always hand its text to the main window.
- Nothing is typed into an app or sent without an explicit action: Replace and Insert type
  into the window the text came from; a reply is inserted, never sent (CLAUDE.md).
- The result is in the language of the input unless the user chose another. When the
  engine's language check still fails after its retry, the screen says so in words; the
  result is never hidden.
- A change is always visible before it is applied: proofread shows a diff, rewrite shows
  the facts it kept, and every change can be undone one at a time.
- Slow is shown honestly: text streams in, the status line says what is happening, and no
  progress bar pretends to know the end. Estimates say "about". While a run goes the result
  pane shows the working pill and, until the first word, the shape of the answer
  (`main-window-busy.html`): the eye is on the result, so that is where the work shows.
- A language is never shown as a flag: it is not a country. Menus show its script instead.
- "On this device" is on every popup; the only network use (model downloads) shows its
  exact source and its check.
- Colour is never the only signal: deletions are struck out, insertions underlined, states
  carry words.
- Keyboard first: numbers pick actions and replies, Enter runs the main action, Esc closes;
  the focus ring is the accent.
- A result on screen always matches the options shown beside it: once something ran on the
  current text, picking another kind, tone or quick/full reply runs it again at once (a run
  still going stops and starts over). Before the first run, options only wait for Ctrl+Enter.
- Dark and light are both first class; screenshots for the owner are taken in both.
- Nothing in the UI mentions accounts or app licensing.
