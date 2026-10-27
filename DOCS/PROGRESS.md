# Progress

Newest first. One entry per working session.

## Milestone status

| Milestone | Status |
|---|---|
| M0 Feasibility spike | done: go (DOCS/models.md, 2026-10-22) |
| M1 Daemon | done (2026-10-22) |
| M2 Main window + popup on X11 + KDE Wayland | in progress: main window, popup, backends built; live desktop checks left |
| M3 Polish + GNOME/wlroots | in progress (owner, 2026-10-27: M3, then the snap first): settings, model manager, My actions, Write, history, autostart and MTP drafting built; GNOME clipboard flow fixed in code; live GNOME check and first-run timing left |
| M4 Mail | todo |
| M5 Release | todo |
| M6 IME + Windows | todo |

## 2026-10-27 - Hindi email greetings, M3 gaps, the snap recipe

Owner asked for M3, then a snap first (2026-10-26). M2's live app checks stay open.

- Format 7 (owner report): a Hindi text formatted as an email got "Hi," and "Thanks,". The
  line that names the text's language now also covers any greeting, closing or heading the
  layout adds, and the closing is "Thanks," alone "with no name line after it". English lines
  added to Hindi emails: E2B 6 to 2, E4B 4 to 1; "[Sender Name]" on E2B 1 to 0. New metric
  `added_english`, 3 new Hindi email items. First output about +0.2 s. DOCS/models.md.
- M3 survey: settings, model manager, My actions, Write, optional history, login autostart
  and MTP drafting were already built during M2. Left: GNOME, the snap, first-run timing.
- GNOME: Wayland sends the clipboard only to the window with focus, and the popup read it
  before it had focus. The clipboard backend now says it `needsFocus`, and the popup reads
  again when it gets focus and on each clipboard change until an action runs
  (`readsTheClipboardAgainOnceItHasFocus`). Not run on a GNOME session yet.
- The command to bind and the login entry now name the package's own command
  (`launchLine`): `paraphrase --popup` in the snap, `flatpak run com.ktechpit.paraphrase`,
  the AppImage path, or `paraphrase-app`. The snap's autostart file goes to
  `$SNAP_USER_DATA/.config/autostart`.
- Packaging: install rules for the daemon, the CLI, the skills and llama.cpp's libraries, with
  the per-CPU libraries in `lib/paraphrase/ggml` and relative RUNPATHs. A staged install ran
  a real proofread. `snap/snapcraft.yaml` and a GitHub workflow that builds it
  (DOCS/packaging.md, 5a).
- The snap on the dev machine (owner install): the app and daemon start, own their D-Bus
  names, the setup window downloaded E2B into `~/snap/paraphrase/common/models` (366 s, sha256
  checked) and a proofread ran. Owner findings, fixed:
  - No taskbar icon: snapd names the desktop file `paraphrase_paraphrase`, so the window's app
    id is now that inside a snap (`desktopFileName`); checked on the taskbar.
  - "Open Paraphrase" at the end of setup closed the app: the main window was asked for while
    setup still counted as open. Test `opensTheMainWindowWhenSetupIsDone`.
  - Paste back showed "You paste": the KWin tracker script was written to the snap's own
    /tmp, which KWin cannot see. It is now one file in the runtime folder.
  - The shortcut showed "By hand" when the desktop had the shortcut but no keys (taken by
    another app). Mocks approved (owner, 2026-10-27): "No keys" with Choose keys, and Change
    keys next to set keys; both open the desktop's own window (ConfigureShortcuts). Setup
    and Settings now follow the desktop live instead of what they read at startup.
  - Proofread's Result tab kept the list of changes under the text, so a long result had
    little room. Mock approved (owner, 2026-10-27, main-window.html): Result shows the
    corrected text alone at full height; the list with Undo is on Changes only. Window and
    popup.
  - "The shortcut opens" in Settings picked an option only from its small circle; the whole
    card picks it now (`picksAnOptionFromAClickAnywhereOnItsCard`).
  - No keys for the snap's shortcut: a test run had left an entry `paraphrase_paraphrase`
    holding Ctrl+Alt+Space; removed.
- Flatpak (owner, 2026-10-27: start Flathub packaging, test locally first):
  `packaging/flatpak/com.ktechpit.paraphrase.yaml` and `scripts/build-flatpak.sh`
  (DOCS/packaging.md, 5b). The KWin tracker script goes to the runtime folder the host shares
  with a Flatpak. Metainfo and desktop file follow the Flathub keyword research
  (FlathubSEO/docs/paraphrase-keyword-research.md): name "Paraphrase", summary "Offline AI
  grammar checker", keywords proofread, rewrite, spell first; screenshots, branding and a
  0.1.0 release entry. Both validators pass; the linter's one error is the KWin permission,
  which needs a Flathub exception. Not built yet: waits for the reply greeting fix (owner).
- The snap workflow now publishes to the Snap Store as Omnidict's does (owner): main to edge,
  version tags to stable, nothing kept on GitHub. It needs the store name and credentials
  from the owner first (DOCS/packaging.md, 5a); until then it only builds.

## 2026-10-26 - The popup opens in the main window with what it shows

Owner report: a long email made the popup taller than the screen, and its quick replies are
one-liners with no way to a full reply. Mock `popup-open-in-window.html`, approved by the owner
on 2026-10-26 (recorded in DOCS/mocks/index.html), and built:
- A long message stays about six lines tall and scrolls inside its box.
- "Open in window" on every popup result page (the expand button the actions page has). The
  main window takes the text, the action, its language and the result as it showed, without
  running again; a run still going stops and the window waits for Run.
- Under quick replies: "Need more than a line? Write a full reply in the window". Reply opens
  on Full reply with the message, the cursor in "What do you want to say?", nothing running.
  Email or chat follows the message's shape until the user has chosen one in the window.
- The hand-over is in memory only; history keeps the run once, where it finished.
- Settings: the scroll bar sits at the window's edge, and the Models table wraps so every page
  fits the window at its opening size (a test checks this with real fonts).
- Bug (owner): after inserting a proofread over one note in a browser canvas, the popup on the
  next note showed the first note's old text: the browser never updated the highlighted text.
  A capture now skips text that a result replaced (DOCS/platform-matrix.md).
- Insert keeps the result on the clipboard (owner, 2026-10-26, changing PLAN's clipboard
  hygiene for Insert only): a paste with no text field in focus had lost the result, since the
  old clipboard came back 0.7 s later. A selection the result replaced still counts as stale, and
  so does the result on the clipboard.
- Full reply without notes no longer retells the message or takes on the sender's plans
  (owner report, 2026-10-26): draft revision 18, eval in DOCS/models.md.
- Tray menu: an icon per item, as in the app's menus (owner request, 2026-10-26). The desktop
  draws this menu, so it gets the theme's icons by name (document-edit, edit-select-text,
  configure, application-exit), with our Lucide icons where a theme lacks them.

## 2026-10-26 - Spacing and consistency pass against the mocks

Every screen (main window, popup, setup, settings; light and dark) compared with its approved
mock at the mock CSS values. No new design: each fix brings a screen back to its mock.
- Main window: your text now has the result's line spacing (15px/1.7; QPlainTextEdit ignored
  line height, so the fields are a plain-text QTextEdit, `TextField`); change rows are 12 px
  with smaller Undo links, 34 px apart; the run button no longer moves by a pixel between labels.
- Shared: a disabled primary button is the filled one faded to 45%, readable in both themes
  (it was white on grey); pills are 22 px with full rounding; the change list's "end" pill is
  round again; reply choices use the normal weight; items in a wrapping row share its centre.
- Layout shifts removed: a chosen option or ticked model card no longer moves its content by a
  pixel when its border thickens.
- Setup: a real checkbox (it drew as a black square), a check for finished steps, grant cards
  16 px apart and flush with their padding, download card padding 16/18.
- Settings: form labels line up with their fields, no empty gap above Cancel/Save, a sunk
  Try it sample, no stray rule or padding on Privacy while history is off, the Privacy intro at
  640 px, the models table inset like the shortcut table, Remove and Turn off aligned.
- Popup: header padding 12, the Esc key cap its own height, 12 px between replies, the result
  box on the panel colour in dark.
- Left for the owner: dark "Delete all" (white on #FF8A80, about 2.2:1) matches the mock but is
  low contrast; a change needs a mock update.

## 2026-10-26 - A run button, and history keeps the window's size

- Mock `main-window-run.html`, approved by the owner on 2026-10-26 (recorded in
  DOCS/mocks/index.html), and built (owner question: Ctrl+Enter is hidden from people who use
  the mouse):
  - A run button at the bottom right of the left pane, named after what it does ("Proofread",
    "Polite", "Into Hindi", "Write 3 replies", "Write the reply", "Write it"), with the action's
    icon and Ctrl Enter as its chip. One width whatever the label; a longer one is cut with "..."
    and shown whole as a tip.
  - Filled while no result for this text and these options is showing; plain once it shows, so
    Copy is the one filled button; filled again after an edit. Disabled while the field it needs
    is empty and while a run goes.
  - Reply and Write open their form with the cursor in it and run only when it is filled. The
    hints say what is missing instead of naming the keys.
- Opening or closing History no longer changes the window's height: writing and history now
  share one stacked area, so the window's minimum never changes between them (it grew because
  both showed for a moment while swapping). `historyKeepsWindowSize` checks it, failing on the
  old code under a real Wayland compositor.

## 2026-10-25 (night) - History, off by default, kept encrypted on this computer

- Mocks `settings-privacy.html` and `main-window-history.html`, approved by the owner on
  2026-10-25 (recorded in DOCS/mocks/index.html) (after one round: the filter pills on one row, a gear on Settings), and built:
  - Settings, Privacy: "Keep a history" (off by default, PLAN D13); keep entries for 7 days,
    30 days (the default), 1 year or until deleted; "Not from these apps" with KeePassXC,
    Bitwarden, 1Password and KeePass from the start; what is kept, where and its size; Delete
    all, asked on the page; turning off asks whether to delete what was kept, keeping it by
    default. Without a keyring the switch stays off and says why.
  - The main window: History beside Settings (Ctrl+H) in place of the panes; entries by day,
    newest first, search through text and result, filters (All, Proofread, Tone, Reply, More);
    the open entry with Copy result, Use again (text, action and language back on the left)
    and delete, asked in its header; the off state with "Turn on history".
  - Kept: each finished run in the window or the popup, with the app it came from; another
    version, Retry, a shorter or politer rewrite and an edited copy update the same entry.
    Stopped and failed runs and the popup's read-ahead are never kept.
- Another version, Retry and "3 new ones" now ask the daemon for `variety`: a sampled skill
  runs at temperature 0.95 with a fresh seed, so the new version differs (owner report: it came
  back almost the same). Checked live on Casual: the variety runs also changed the sentence's
  shape, not only a word.
- Full reply 12: notes that begin mid-thought ("until we fix X, you can do Y") no longer turn
  into a refusal (the owner's .de email case). Signing with the greeted name was tried and
  dropped: the small model then wrote "[Your Name]". Numbers in DOCS/models.md.
- ADR-017: an SQLite file in the app's data folder; text, result and options sealed with
  AES-256-GCM, the key from the desktop's keyring (Secret portal, else Secret Service) and a
  salt in the file. Tests: `tst_history` (sealing, a wrong key, delete all, nothing in clear on
  disk) and history cases in the window, settings and popup tests; screenshots of both screens
  in light and dark compared with the mocks.
- Not checked live: turning history on against the real keyring, which stores a secret in the
  owner's KWallet; left for the owner. Still open: E2B offers a PIN to the Hindi scam item in
  about 1 run in 10 (revision 11 the same).

## 2026-10-25 (evening) - The popup moves, shows it is working, and Tone keeps a chosen language

- Mock `popup-working.html`, approved by the owner on 2026-10-25, and built:
  - the header moves the popup (a grip at its left; the desktop's own move, so Wayland too);
  - the actions page shows the working pill from the first second: "Loading the model",
    "Reading the text for Tone", then "Ready for Tone" (from the daemon's Status, polled while
    it reads ahead);
  - a language the user chose is a pill beside the result ("हिन्दी · Hindi ×"); × reruns in
    the text's own language; the next tone keeps the chosen one;
  - a tone's line says what it did: "Elaborate · kept: Friday, Thai · 21 to 34 words";
  - Settings, Shortcut: the shortcut opens the popup (the default) or Paraphrase's window with
    the selected text in it.
  Checked live in KWrite: dragged 300 px left and 150 px up, the footer's three steps, Hindi then
  Elaborate then ×, and the shortcut opening the window.
- Tone 11: a tone in another language is applied, not only translated (owner report: Elaborate in
  Hindi read like a translation). Numbers in DOCS/models.md.
- Facts check: across languages it counts numbers, not names (no more "check: Friday, Thai are
  missing" on a Hindi result).
- The popup tests wrote to the user's real settings (and so to which action is highlighted);
  they use a temporary folder now, and the old popup counters were removed from the owner's
  settings file (a backup was kept).

## 2026-10-25 - The popup's fixes checked live; the daemon comes back after its idle exit

- Live check on KDE Wayland of the popup fixes from 2026-10-23: the popup fits its content,
  number keys open the Tone menu on its first item, the language list scrolls (End reaches the
  last language), and a run shows the working pill and the shape of the answer in the result's
  own outline, the answer then about as tall.
- Found on the way: after 30 quiet minutes the daemon exits (`--idle-exit`) and the app's
  client still counted itself connected, so the popup said "Paraphrase is not running" and a
  run in the main window would fail. Before each call the client now starts the daemon again
  when its name is gone (or the private daemon has ended). Checked live: the daemon stopped,
  the shortcut opened the popup with its actions and a new daemon. A longer status line in the
  popup wraps instead of stopping mid-sentence.

## 2026-10-24 - Full reply never offers a password, code or card details

- Full reply (draft 11): a reply to a phishing message no longer offers to send card details,
  passwords or codes. Revision 10 did so in 9 of 70 runs on E4B and 32 of 70 on E2B, mostly
  because "say I will send it or check" was the advice for what only the user knows; revision 11
  in 1 and 3 of 80, without touching ordinary replies. Numbers and rejected wordings in
  DOCS/models.md. Quick replies were already safe.
- Eval: eight new scam items in English, Hindi and Hinglish, and patterns that catch an offer to
  send a secret while letting a refusal pass, with a unit test.

## 2026-10-23 (late night) - The popup checked live, Write 7 note titles

- The popup driven live on KDE Wayland (KWrite as the source, the owner's shortcut), compared
  with popup-actions.html and popup-proofread.html. Fixed:
  - the source app by its name ("from KWrite", "Replace in KWrite"), from its desktop file,
    not its id ("org.kde.l..."); a long name is cut with "…", never mid-letter;
  - the popup fits its content: no empty band under the actions (the page stack took the
    result page's 565 px), no jump from 567 px when a result comes;
  - the working pill and the shape of the answer while it runs (main-window-busy.html), sized
    to the text; retries show; the caret while words stream;
  - every proofread change listed with a short reason ("spelling", "capital letter",
    "question mark"), as popup-proofread.html note 3 asks; before, two rows of five showed;
  - Replace typed nothing: it waited for the source window to get the focus back while the
    popup kept it. The popup now steps aside first, and comes back with "on the clipboard"
    if the text could not go back;
  - on Wayland the popup came up behind the app in use when the tray copy opened it: the
    shortcut portal's activation token and the launcher's `XDG_ACTIVATION_TOKEN` (passed on by
    `paraphrase-app --popup`) now bring it to the front;
  - a menu opened from its number key starts on its first item.
- KDE asked again for the "Control input devices" permission: the saved token was granted when
  KDE still filed the app under Konsole. Approved again by the owner under its own id.
- CLI: `paraphrase batch` crashed on exit now and then (a queued D-Bus event dropped while the
  client was destroyed); it delivers them first.
- Write 7: a note opens with a short title saying what it is about, not a date line ("Date:
  Unspecified"). Numbers in DOCS/models.md.
- Owner's calls: the popup highlights the action used last (DECISIONS.md, the read-ahead
  record). Language lists (Translate, "Write it in another language") show about twelve
  languages and scroll, instead of covering the screen.
- Mock `setup-desktop.html`, approved by the owner on 2026-10-23, and built: setup starts with
  "Over any app", the shortcut and "Put the result back"; Allow opens the desktop's "Control
  input devices" dialog then, not at the first Replace; both optional ("Skip for now"). Settings,
  Shortcut shows the "Put the result back" row too.

## 2026-10-23 (night) - The result pane shows its work, menus with icons, Write 6

- Mocks `main-window-busy.html` and `menus.html`, approved by the owner on 2026-10-23.
- App: while a run goes, the result pane shows a working pill with the step in words
  ("Reading your text", "Writing", "Checking, writing again", "Writing 3 replies") and the
  seconds so far, and until the first word the shape of the answer (lines with a shimmer, three
  cards for quick replies); the text then streams with a blinking caret. A rewrite of the result
  (Shorter, Make it polite) keeps the result and shows only the pill. Owner: "the only way to
  understand if the app is doing something".
- App: Tone, Format and My actions menus have an icon per item (the user's own actions their
  first letter), the Tone picker in Write and Full reply shows the tone's icon, and Translate
  shows each language's script and its name in its own writing, never a flag. The "Best effort"
  heading shows its text again.
- Write 6: no "[Your name]" under a note, message or post (owner report). Numbers in
  DOCS/models.md.
- GitHub repository created (private for now): https://github.com/keshavbhatt/paraphrase, linked
  from the AppStream data. The dev run installs a desktop file, so the desktop's shortcut
  settings list the app as "Paraphrase (dev)" instead of the terminal that started it.

## 2026-10-23 (evening) - Full reply keeps the sender's plans theirs, Write signs with the name, options run again

- Owner report: "Thanks Gabri, cool idea" as the note on a long announcement that ended in the
  sender's own plan came back with that plan written as the user's. Draft 10 adds one sentence:
  a note that only thanks or praises gives a reply that does only that. Numbers and the rejected
  wordings in DOCS/models.md "Prompt revisions"; four new hard eval items of this kind.
- Write 5: an email signs with the name the description gives ("Best regards, Keshav Bhatt")
  instead of "[Your name]", and capitals are fixed ("Monday"). Numbers in DOCS/models.md.
- App: a result always matches the options beside it. Once something ran on the text, a new
  "Write it as", email or chat, tone, or quick or full reply runs again at once (a run still
  going stops and starts over); before the first run they wait for Ctrl+Enter (owner: picking
  Social post did nothing until Ctrl+Enter). Rule in DOCS/DESIGN.md section 4. The main window
  tests now keep their settings in a temporary directory.
- Known: E4B once wrote "I will check and send my card details" to a phishing message
  (dr-en-104), as revision 9 did on E2B; the item has no pattern that catches it yet.

## 2026-10-23 (afternoon) - Skill prompts read once per skill, the last mock details

- Engine (PLAN 5.3, per-skill prompt cache): four extra cache sequences keep the last system
  prompts; a skill that comes back after another shares its prompt back instead of reading it
  again, drafted skills in the draft head's cache too. On E2B after switching: Proofread's first
  output 6.5 s to 0.6 s, Reply 2.6 s to 0.4 s, Summarize 3.7 s to 1.0 s. The cache grows by
  4 x 1024 cells (peak memory about 0.1 to 0.3 GB more). Reused cells change rounding as the
  ordinary prefix reuse already did; the drafting test compares runs from an empty cache.
- Skills: draft 9 (shape before the message for read-ahead, dates as written), Write 2 to 4
  (no placeholders but the mock's "[Your name]", greetings in the text's language), Summarize 5
  (reference numbers and dates kept). Numbers in DOCS/models.md "Prompt revisions".
- App, against the approved mocks: 6 px between icons and text on plain buttons, the full
  reply's language pill and Paste beside the message's label, popup replies without a copy
  button per row and "Edit before sending".
- Left: E2B writes "[Date]" in one Write note heading; Write cannot answer in another language
  than the description's (the engine keeps output in the input's language); Format's last
  E2B slips (one translated table, one "[Sender Name]").

## 2026-10-23 - New skills, Full reply fix, screens matched to the mocks

- Owner approved the three mocks built before approval (2026-10-23): main-window-compose.html,
  settings-actions.html (with Delete asking "Delete?" at the same width) and
  settings-general-shortcut.html.
- Owner decided the Flatpak ID: `com.ktechpit.paraphrase` (lowercase, as the code and
  DOCS/packaging.md already use); PLAN section 13 is updated.
- Skills (eval reports in `eval/reports/`, numbers in DOCS/models.md "Prompt revisions"):
  summarize (revision 4, quality), format bullets, table and email (revision 5, fast),
  translate (revision 2, quality), draft email and chat (revision 5), custom for My actions
  (revision 2) and compose email, message, post and note (revision 1, measured once, no
  before). Datasets for each in `eval/datasets/`, with 8 new hard draft items written from the
  owner's report (short "no", "yes" and "done" notes in English and Hindi).
- Full reply: the owner's web.de email was answered as if the note were the sender's problem.
  Draft revision 5 frames the note as the user's own answer; it costs about 3 s more before
  the first word on E4B. Per-skill prompt caching would take that back (next).
- The client ends a running request with `daemon-gone` when the daemon stops; an eval run had
  waited 6 hours on a dead private daemon.
- Screens against their mocks: 14 px base text (the window rule did not reach its children),
  44 px big buttons (QWidget's own `size` property swallowed the dynamic one), slim switches
  and pills, one pane head height, line height and insets, the popup sized to its page and
  text, settings headings, dividers and the Shortcut table. Chips keep their labels and the
  choice shows in the result header; Retry/Stop, Delete and the tone button keep one width.
  The My actions editor keeps its gaps and a long Try it result scrolls (capped at 160 px).
- Left: icon-to-text gaps on plain buttons (2 px, mock 6), the full-reply pill and Paste in
  the body as the mock shows, the popup reply rows without per-row copy.

## 2026-10-23 - My actions, General and Shortcut settings, both models loaded, facts kept

- My actions (mock settings-actions.html, built before approval): skill schema v5
  `user_presets`; the daemon keeps the user's actions in actions.json next to its settings
  (`ListActions`, `SaveAction`, `DeleteAction`, the `SkillsChanged` signal) and runs an unsaved
  one through the template with its `instruction` and a `tier` option (Try it). Settings has the
  list, the editor and Try it; the window and the popup show My actions with "New action...".
- Settings General and Shortcut (mock settings-general-shortcut.html, built before approval):
  start at login (autostart entry, or the Background portal in a Flatpak), keep running in the
  tray, appearance (system, light, dark); what this desktop lets Paraphrase do, in plain words,
  and the command to bind by hand. The notice's link opens the Shortcut page.
- The fast and the quality model stay loaded together when memory holds them with 6 GB to
  spare: on this laptop, proofread after a tone went from a reload (6 to 15 s) to 1.2 s.
- Rewrites and formats say which names, numbers and dates they kept ("kept: Sam, Friday") and
  warn in yellow when one is missing; the popup offers "Write it in another language"; proofread
  with nothing to fix says "Looks good, no changes" and offers only Close.
- Desktop entry with a "Work on the selected text" action, the app icon and AppStream metadata
  (PLAN 13 wording; the homepage URL is left out until there is one).
- `tst_real_models` (by hand, `PARAPHRASE_TEST_REAL=1`) drives every action through the window
  and the popup with the real models and saves screenshots; it found the window widening with a
  long action label and a stale status line, both fixed.
- Next: per-skill prompt caching in the engine (proofread then reply on the same model still
  reads ~600 prompt tokens, about 4 s on this CPU; keep each skill's system prompt in its own
  KV sequence and copy it in); the first-time portal dialogs, checked by a person; GNOME and
  wlroots (M3); opt-in history (M3, needs a mock).

## 2026-10-23 - M2: every main-window feature, the popup, setup and settings, desktop backends

Owner, before sleeping: autopilot; screens without an approved mock get a mock, are built, and
are flagged for review; quick replies stay on E2B; desktop tests type only into windows the
tests open; up to about 15 GB of model downloads allowed.

- Main window matches the approved mocks (header and notice bands, key chips, pane heads,
  change chips, reply cards) and has every skill as an action: Summarize, Format (menu),
  Translate (language menu, the text's own language left out), and Reply with Quick replies
  or a Full reply (mock main-window-reply-draft.html, approved 2026-10-23): what you want to
  say, email or chat message, a tone, one editable draft with versions, Another version and
  Shorter.
- Owner: no big single-file classes. The app is split by responsibility: `ui/core` (skill
  catalog, request runner, result summary, languages; no widgets), `ui/common` (theme,
  widgets, chip button, flow layout), `ui/parts` (action bar, input pages, result views,
  change list: shared by the window and the popup), `ui/main`, `ui/setup`, `ui/settings`,
  `ui/models`, `popup/`, `integration/`, `shell/` (Application, AppFlow, tray, App service).
- First-run setup and Settings (mocks setup-model.html and settings-models.html): hardware
  chips, Fast and Quality cards, download with progress, pause and the exact source, Try it;
  Settings with the Models page (tier pickers, installed models with Remove after a
  confirmation, speed-up switch, languages); daemon `SetTierModel` and `SetSpeedUp`.
- The popup (mocks popup-*.html): actions numbered 1 to N with reading ahead for the most
  used one, proofread diff with Undo per change, three replies, tone presets, Replace or
  Insert reply only on click, Copy, Retry, Esc closes, Open in window.
- Desktop backends behind interfaces (DOCS/platform-matrix.md): GlobalShortcuts portal and
  X11 key grab; data-control capture of the selection on KDE Wayland and PRIMARY on X11;
  paste through the RemoteDesktop portal or XTest with the clipboard restored, only into the
  window the text came from (KWin script or _NET_ACTIVE_WINDOW), else the clipboard; the
  clipboard and "bind `paraphrase-app --popup`" as fallbacks everywhere.
- One running copy on the session bus (`com.ktechpit.paraphrase.App`), a tray icon, one
  daemon connection for the whole app.
- Daemon: `ListSkills` reports each skill's main input and request inputs; requests may carry
  `intent` and `tone`; inputs follow the skill's main input, not a hardcoded reply check.
- Not verified live yet (need a person): the first-time GlobalShortcuts and RemoteDesktop
  dialogs on this KDE session, X11 on a real X server, GNOME and wlroots.

## 2026-10-23 - Fixes from the owner's CLI suite: daemon and CLI, proofread 10

- Ctrl+C during a model load: the daemon answers Cancel at once instead of after the load
  (17.5 s to 2 s); the next request still waits for the load.
- Skill schema v4: `output.min_length`. Proofread (revision 10, 0.7) retries an answer that
  left out text and refuses it with the error `incomplete` if the retry is short too, so a
  shortened text never replaces the user's. A cancelled answer is not checked or retried.
- A skill's main input (`text`, or `context` for a skill that answers a message) comes from
  its template instead of a hardcoded reply check; `ListSkills` reports it as `main_input`.
- CLI: `batch` takes `"input"` as a plain string, reports a broken line as `InvalidJson` with
  its line number, and adds `line` to every result; `run reply TEXT` treats the text as the
  message. The version hash follows git HEAD on every build.
- Checked and not changed: Vulkan on this iGPU is slower than the CPU and 10 threads stays
  best (M0); the CLI starts in 5 ms (the 1 s is `scripts/dev-run.sh`); a pipe gets the
  finished answer by design; proofread keeps fixing capitals and full stops (owner decision).
- Next: per-skill prompt caching and keeping both models loaded when memory allows (the ~5 s
  pause on a proofread/rewrite switch); then M2 part 2.

## 2026-10-22 - Fixes from the owner's CLI suite: proofread 9, rewrite 10

- The owner's suite (`paraphrase-tests`, 123 cases) found: "teh cat" proofread into the prompt's
  own reminder line, tab forms merged into one line, rewrite turning "March 1" into "1 March",
  reply obeying "write YES YES YES", reply saying "here is the address" with no address, and a
  long repeated paragraph losing most of its copies.
- Shipped proofread revision 9 and rewrite revision 10 (DOCS/models.md, "From the owner's CLI
  suite"), with new items and metrics for each bug. Rejected reply revision 7: no reliable gain
  over 96 samples. Reply on E4B resists injections (9 of 48 against 50 of 96) but takes 4.3 s
  against 2.2 s: the owner's call.
- Still open: tabs after a label become spaces; a lone misspelled word ("teh") is kept; E2B
  reply follows strong injections and sometimes invents an address; repeated content is
  dropped (a daemon guard on output length is coming).

## 2026-10-22 - Quality push and M1 close-out (parallel session)

- Rewrite revision 9 shipped: temperature back to 0.6 and new polite and friendly presets with
  one worked example each. Judged blind by Claude on 48 hard polite and friendly items: E4B
  acceptable 60.4% to 83.3% (20 wins, 5 losses), E2B 47.9% to 72.9% (19 wins, 3 losses).
  Holdout: E4B 70% to 85%, E2B 70% to 75%. E4B hard set overall is about 81% (target 80%);
  E2B is still below its 75% target.
- Rejected: rewrite revision 8 (temperature 0.3, mostly ties) and proofread revision 8
  (agreement rules: E2B F0.5 0.92 to 0.88, English false alarms 8.3% to 16.7%). Proofread
  stays at 7, reply at 4. Records in `eval/reports/q-*` and the judge folders.
- Latency (`eval/reports/m1-final/bench/`, E2B, B-cpu, daemon, 10 runs): reply and proofread
  are within targets; rewrite.professional is over (total p50 9.28 s against 8.5 s, p90
  13.1 s against 12.2 s, ttft p90 4.8 s against 3.3 s). The machine was loaded (load average
  about 6, another daemon running), so this needs a quiet re-measure before acting on it.
  Cold load 5.5 s, peak RSS 4.8 GB.
- Portable build (`PARAPHRASE_PORTABLE_CPU=ON`) builds all targets and ships the
  libggml-cpu variants; the portable daemon ran 5 proofread items correctly
  (`eval/reports/q-portable/`).

## 2026-10-22 - M2 step 1: the standalone main window

- M1 is done: the CLI works for proofread, reply and rewrite, the eval runs through the daemon
  (`eval/reports/m1-daemon/`, same quality as M0) and the unit tests pass.
- `client/` (new top-level library, moved out of the CLI; PLAN's tree had it under
  `app/src/client`): the daemon client and the word diff, shared by the CLI and the app.
- `paraphrase-app` (ADR-013, mocks main-window.html and main-window-reply.html): actions
  come from the daemon's skills (Proofread, a Tone menu with the rewrite presets, Smart reply,
  Ctrl+1 to Ctrl+3); results stream in; proofread shows the word diff with Undo per change
  and a Changes/Result switch; reply shows three cards with copy, an edit box, 3 new ones and
  Make it polite; Copy and Use as my text; Retry and Stop; light and dark from DESIGN.md.
  tst_main_window drives it against a real daemon service with a fake engine; with
  `PARAPHRASE_SCREENSHOTS=<dir>` it saves the window in both themes.
- Differences from the mock: Summarize, Format, Translate and My actions are not shown
  because those skills do not exist yet; no reason per change, because proofread returns only
  the corrected text; Settings waits for M3; the notice text says what works without a
  shortcut.
- Lint now runs clang-format 23 and clang-tidy 23 inside the core24 SDK
  (`scripts/lint-cxx.sh`), because the host's GCC 16 headers broke clang-tidy.
- Next: tray, hotkey, capture and inject backends and the popup, on X11 and KDE Wayland.

## 2026-10-22 - Quality first: reply revision 4, rewrite revision 5

- Hard and holdout sets for reply (36 items) and rewrite (34 items, run in five tones),
  written by Claude; a reply metric for continuing the user's draft; blind A/B judging by
  reviewer agents (`eval/judge.py`, results in `eval/reports/judge/`).
- Reply revision 4 (E2B): acceptable 64% to 80%, draft continued 0% to 100%, holdout 6 to 8
  of 10. Rewrite revision 5: acceptable 55% to 75% on E4B and 48% to 68% on E2B, changed facts
  44 to 4 and 53 to 9; holdout 50% to 74% (E4B) and 47% to 70% (E2B). DOCS/models.md.
- Rejected after judging: reply revision 5 (a tie) and rewrite revision 6 (a tie on E4B,
  worse on E2B); their outputs and verdicts stay in `eval/reports/` for the record.
- Next: the polite and friendly presets on their own; reply's injected instructions; then
  proofread's remaining Hindi agreement misses.

## 2026-10-22 - Quality first: proofread revision 7

- Owner: the latency benchmarks wait until the models write correctly ("we can't go out with
  a broken product which is super fast"); Claude writes the test samples, hard ones included,
  and owns the model choices. The M1 bench run was stopped; the eval it had finished through
  the daemon (`eval/reports/m1-daemon/`) matches M0's quality, which confirms the daemon.
- Owner decision: proofread fixes like a proofreader (capitals, full stops, question marks).
  Four references that kept chat style were updated.
- New sets: `*.hard.jsonl` (43 proofread items, the owner's CLI sentences among them) and
  `*.holdout.jsonl` (25 items, never tuned on). DOCS/models.md, Datasets.
- Proofread revisions 5 to 7 (DOCS/models.md, Prompt revisions). E2B, all 124 items: F0.5
  0.82 to 0.92, recall 0.68 to 0.91, exact 57% to 78%, no more changes to correct text;
  holdout: F0.5 0.56 to 0.91. All three injected instructions are now corrected, not obeyed.
  E4B over-edits with the same prompt, so proofread stays on E2B.
- Also: the daemon loads a portable build's per-CPU llama.cpp libraries from a known folder
  (ADR-008), so a packaged daemon finds its CPU backend.
- Next: rewrite and reply quality, the same way (hard and holdout sets, revisions measured
  before and after); then proofread's remaining Hindi agreement misses.

## 2026-10-22 - M1 step 7: the CLI

- `paraphrase proofread | rewrite --tone T | reply [--draft D] | run SKILL`, with `--to`,
  `--json` and `--no-stream`, plus `skills`, `status` and `models list | download | pause |
  delete` (README, "Using the CLI"). Text from the arguments or standard input; answers stream
  into a terminal, a pipe gets the finished answer; reply prints one suggestion per line.
  Ctrl+C cancels the request (exit 130).
- It finds the daemon on the bus, starts it detached, or runs it privately (ADR-010).
  tst_cli runs the real binary through the private path with the real model (proofread, reply,
  rewrite with JSON); by hand on the session bus: started the daemon, streamed a rewrite in a
  terminal, reused the daemon, Ctrl+C left it idle.
- Fixed on the way: the detached daemon wrote its log into the CLI's terminal, mid-answer
  (now discarded); `--to` was silently ignored by reply and proofread, which cannot write in
  another language (now refused for any skill whose prompt does not take it).
- Observed: rewrite runs on E4B on this 30 GB machine (quality tier, ADR-016), so switching
  between proofread and rewrite reloads a model (about 6 s each way). To weigh in step 8.
- M1 acceptance: proofread, reply and rewrite work from the CLI; tests green. Left: step 8,
  eval reports and the latency bench through the daemon, and the portable-build re-bench.

## 2026-10-22 - M1 step 6: the D-Bus API

- `paraphrase_api` and `paraphrase-daemon`: the API in DOCS/api.md on the session bus, or on
  a private peer socket (ADR-010, `--private-socket`) with the same object. Requests queue for
  one inference thread; a new request from a client cancels its earlier ones; the skill's tier
  picks the model; long input is refused; `Warm` reads ahead (ADR-005); the model unloads after
  10 quiet minutes and the daemon exits after 30 (never during a download).
- Streaming signals go only to the client that asked (targeted D-Bus signals): checked on the
  real session bus, where `gdbus monitor` saw none of a request's text.
- Models over D-Bus: list (with source, revision and sha256 for the UI), download with its
  draft head and throttled progress, pause, delete (refused while in use).
- Engine: an `Engine` interface (tst_api runs the real service with a fake), read-ahead,
  token counting, unload, translatable errors, and a cancel that holds until the next job, so
  one that arrives between the two calls of a language retry still counts.
- Measured on machine B through D-Bus: idle daemon 20 MB (target under 40 MB); a proofread on
  E2B with its draft head, 10 threads: 2.2 s.
- Next: step 7, the CLI (`paraphrase proofread < file`), which starts the daemon or runs it
  privately.

## 2026-10-22 - M1 step 5: models

- Catalog schema 2 (DOCS/models.md, Catalog): Gemma 4 E2B is the fast-tier model and E4B
  the quality-tier one, with the memory they need and use, and their draft heads. The daemon
  builds the catalog in and never fetches it (ADR-015); the eval harness reads the same file.
- `paraphrase_models`: the catalog reader (strict; file names, repositories and checksums are
  checked because they become paths and URLs), the models folder (ADR-009 paths, the verified
  stamps of scripts/fetch_model.py, so development models count as installed), the hardware
  probe and the tier choice (ADR-016), and the downloader (one model at a time, `.part` files
  resumed with Range across restarts, sha256 on the thread pool, HTTPS only).
- Probe on machine B: 10 threads of 14 CPUs, the M0 sweep's fastest; it leaves out the two
  low-power cores. tst_engine now uses it and runs in 49 s instead of 84 s.
- The quality model is offered from 16 GB; on smaller computers rewrite runs on the fast one.
- Checked once by hand: the downloader fetched the E2B draft head (59 MB) from Hugging Face,
  followed its redirect and verified it (`PARAPHRASE_NETWORK_TESTS=1`; the suite stays offline).
- Open for step 6: the engine's few error messages are plain strings; they need `tr()` when
  they reach clients through the API.
- Next: step 6, the D-Bus API (service, request queue, inference thread, idle unload, private
  fallback).

## 2026-10-22 - M1 step 4b: MTP drafting per skill

- The engine loads the model's MTP draft head as llama-server does (`--spec-type draft-mtp`,
  default draft length 3) and drafts requests whose skill says `speculative: draft`
  (proofread, rewrite; ADR-004). A reply in between does not break the cache: the engine
  tracks what the draft context holds and re-reads the prompt where the two differ.
- Greedy answers are identical with and without drafting (tst_engine, 10 prompts).
  Proposals accepted on those prompts: proofread 74%, rewrite 30%; llama-server with the same
  head on the same prompts: 65% and 29%.
- Fix found on the way: llama-server feeds the prompt to the sampler, so the repeat penalty
  counts words from the prompt. The engine did not, which changed rewrite and reply answers
  (proofread has no penalty). With the fix, greedy rewrites match llama-server's for the same
  cache state.
- Memory 4.71 GB with the head (4.49 GB without), load 4.8 s (3.5 s without), machine B.
- Model note: greedy E2B writes a malformed Tamil word in one rewrite vector (rw-ta-001), in
  llama-server too; rewrite samples at temperature 0.6 in use. Recorded for the M1 eval.
- Next: step 5, models (catalog, downloader, hardware probe with the thread count, tier).

## 2026-10-22 - M1 step 4: the engine

- `paraphrase_engine`: `LlamaEngine`, the only code that includes llama.cpp. It builds prompts
  as llama-server does (the model's chat template, thinking off) and matches its tokens for
  all 15 vector prompts (tests/vectors/prompt_tokens.json, from eval/export_prompt_tokens.py).
  One deliberate difference: user text is never parsed for control tokens, so text that spells
  out `<|turn>` stays plain text. Prompt prefix cache (a repeat reuses all but the last
  token), the skill's GBNF grammar, streaming of whole characters, an output limit, cancel,
  and variants decoded in one batch.
- `SkillRunner`: a port of eval/paraeval/engine.py. Checks the language of every output,
  retries once with a line naming the language (temperature 0.3 for greedy skills, seed
  +7919), and returns a second failure with a warning (ADR-006).
- Measured on machine B with E2B: peak memory 4.49 GB with the full-size sliding-window cache,
  below llama-server in M0 (4.5 to 5.1 GB).
- Found: llama.cpp's default thread count on this hybrid Intel CPU is 2 (performance cores
  only), where the M0 sweep found 10 fastest. The hardware probe (step 5) picks the threads.
- Next: step 4b, per-skill MTP drafting (ADR-004); then step 5, models.

## 2026-10-22 - M1 steps 1 to 3: tooling, language logic and skills in C++

- Tooling as Omnidict (owner): DOCS/CODING_STANDARDS.md, .clang-tidy, scripts/dev-build.sh and
  dev-run.sh against the KDE Qt snap SDK and runtime, Makefile (build, test, lint, format) and
  CI (.github/workflows/ci.yml). The SDK library path is no longer exported for cmake: it
  breaks the host's cmake on Arch (libnettle.so.8), and the SDK's moc runs without it.
- Step 1: CMake at the repository root, llama.cpp linked as a library (native for dev,
  portable for packages, ADR-008), headers of third-party code treated as system headers.
- Step 2: `paraphrase_util` with the language detector and check. The word lists and names
  moved to daemon/data/language.json, read by both the daemon and the Python harness; the
  Python code stays the reference and eval/export_vectors.py writes tests/vectors/. C++
  matches all 774 detection and 429 check vectors.
- Step 3: yaml-cpp 0.9.0 (ADR-014) and `paraphrase_skills`: registry, schema v3 loader with
  PyYAML's scalar rules, template rendering with fences, request inputs. C++ matches all 11
  skills, 254 input builds, 293 rendered prompts and 12 template cases; broken files (bad
  YAML, unknown values, a grammar outside skills/) are rejected by name.
- Next: step 4, the engine (model, chat template, prompt cache, grammar, streaming,
  variants, cancel, language retry, per-skill MTP drafting).

## 2026-10-22 - Mocks approved, packaging decided, M1 started

- Owner approved all eight mocks (DOCS/mocks/index.html): main window proofread and reply,
  popup actions, proofread, reply and change tone, first-run setup, settings.
- Packaging analysis (DOCS/packaging.md) and ADR-007 to ADR-013: app ID
  `com.ktechpit.paraphrase`, portable llama.cpp build, data folders per format, clients start
  the daemon, D-Bus as the API, one package per format, standalone first (owner: every
  feature works without desktop integration). PLAN D14, D15, M2 and M5 updated.

## 2026-10-22 - M0 decided: go with Gemma 4 E2B (fast) and E4B (quality)

- Owner's answers: quality-tier tie to Gemma E4B (family consistency with E2B); apply
  every speed lever before judging machine A; Hindi/Hinglish floor over all 15 items per
  language at 10%; an engine-level language check; rewrites keep the input language. The
  owner then asked Claude to decide the open product questions (ADR-003 to ADR-006).
- Spot-check: Claude rated all 100 answers in two passes plus a final check (3 changes);
  no human rated. E4B 14/5/1, Qwen 4B 14/4/2, E2B 11/7/2 (good/ok/bad).
- Speed levers measured (DOCS/models.md, Latency): only the Gemma MTP draft heads help
  (proofread -26% on A, -42% to -46% on B); n-gram drafting, Q4_K_M, Vulkan on the iGPU,
  more threads, E2B as a draft model and batched reply variants do not. llama-server
  v0.7.0 ignores per-request speculative settings, so the bench times reply and the other
  skills on two server setups; the daemon switches per request (M1).
- Strict result: the fast tier on machine A misses twice the old targets (first token about
  3 s against 1.6 s). ADR-003 ships it anyway with targets from measurements (PLAN 5.4,
  `eval/targets.json`, `bench_latency.py --check`).
- Engine (`eval/paraeval/engine.py`): language check with one retry that names the language
  (E2B Hinglish rewrites into English: 5 of 7 before, 1 of 7 after), per-skill speculative
  decoding (skill schema v3), batched variants. Both models pass the floor (2% and 7%);
  Claude read all 90 Hindi and Hinglish outputs, none garbled.
- Other languages: the detector called every language English or Hindi; it now names about
  40 and says "its own language" when unsure, without changing any existing item's
  detection. Eight-language smoke set: 48 of 48 outputs kept their language for both models.
- Mocks (DOCS/DESIGN.md, DOCS/mocks/, proposed, waiting for the owner): popup actions,
  proofread, reply, change tone, first-run setup, settings. Also on a design canvas.
- Waiting on the owner: mock approval; human-written eval items; a real older laptop for
  the bench before M5.

## 2026-10-22 - M0 measurements

- llama.cpp v0.7.0 built CPU only and CPU + Vulkan. Five candidates and the judge fetched by
  pinned revision and sha256 (`models.json`, `DOCS/models.md`).
- Eval harness, three skills (proofread, reply, rewrite with 9 tones) and 195 project-written
  eval items. No public dataset is used.
- Prompt revision 1: models answered English text in Hindi (Qwen3.5-4B rewrote 49 of 50
  English texts into Devanagari). Revision 2 removed Hindi from the system prompts and names
  the detected language in the user message: English drift went to 0 for every model.
- Revision 2 left two problems: proofread changed correct casual text (up to 6 of 12 clean
  items), and the professional rewrite translated Hinglish into English (3 to 5 of 7).
  Revision 3 addresses both; its eval, the judge (Sarvam-30B, tier-relevant pairs only) and
  the latency bench run overnight.
- Latency (revision 2, `eval/reports/m0-v2/summary.md`): no candidate meets the PLAN 5.4
  totals on A or B. The iGPU through Vulkan is slower than the CPU on this laptop.
- Waiting on the owner: the results table, the spot-check ratings, then the go/no-go and
  the revised latency targets.

## 2026-10-21 - M0 started

- Owner answers for M0 (recorded in `PLAN.md` section 11):
  - Q4: 8 GB CPU-only (machine A) is a must-have for the fast tier only (proofread, reply,
    format). The quality tier may be labelled "16 GB or GPU recommended" with a slower
    fallback on A. A no-go on A for the fast tier is fatal.
  - Machines: B is the development laptop (Intel Core Ultra 7 255U, 30 GB RAM, Intel
    Graphics (Arrow Lake) iGPU through Mesa Vulkan). A is emulated on the same laptop and its
    numbers are labelled "optimistic (emulated)"; re-run on a real older laptop before M5.
  - Q6: Hindi and Hinglish are a tie-breaker with a hard floor: reject a model that answers
    Hindi in English, breaks Devanagari, or rewrites Hinglish into English.
  - Q2: licensing as Omnidict, with the licensing module carve-out (ADR-002).
  - Eval data written for the project (no JFLEG in the repository), with room for about
    20 human-written items per skill in separate files, scored separately.
  - Judge: a model from a different family than the candidate it scores, blind pairwise,
    plus hard checks. The owner's spot-check of 20 outputs per model is the final word.
- Scaffold: git repository, licence files, editor and format config, Python tooling.
