// Mock helpers: the Lucide glyph sprite, the theme and OS chrome switch, and the review
// toolbar. Nothing here ships in the app. Glyph paths are from Lucide (ISC licence), as in
// Omnidict's mocks.
(function () {
  const G = {
    'x': '<path d="M18 6 6 18"/><path d="m6 6 12 12"/>',
    'back': '<path d="m12 19-7-7 7-7"/><path d="M19 12H5"/>',
    'forward': '<path d="M5 12h14"/><path d="m12 5 7 7-7 7"/>',
    'down': '<path d="m6 9 6 6 6-6"/>',
    'left': '<path d="m15 18-6-6 6-6"/>',
    'right': '<path d="m9 18 6-6-6-6"/>',
    'copy': '<rect width="14" height="14" x="8" y="8" rx="2" ry="2"/><path d="M4 16c-1.1 0-2-.9-2-2V4c0-1.1.9-2 2-2h10c1.1 0 2 .9 2 2"/>',
    'refresh': '<path d="M3 12a9 9 0 0 1 9-9 9.75 9.75 0 0 1 6.74 2.74L21 8"/><path d="M21 3v5h-5"/><path d="M21 12a9 9 0 0 1-9 9 9.75 9.75 0 0 1-6.74-2.74L3 16"/><path d="M8 16H3v5"/>',
    'check': '<path d="M20 6 9 17l-5-5"/>',
    'lock': '<rect width="18" height="11" x="3" y="11" rx="2" ry="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/>',
    'pen': '<path d="M21.174 6.812a1 1 0 0 0-3.986-3.987L3.842 16.174a2 2 0 0 0-.5.83l-1.321 4.352a.5.5 0 0 0 .623.622l4.353-1.32a2 2 0 0 0 .83-.497z"/><path d="m15 5 4 4"/>',
    'undo': '<path d="M3 12a9 9 0 1 0 9-9 9.75 9.75 0 0 0-6.74 2.74L3 8"/><path d="M3 3v5h5"/>',
    'stop': '<rect width="18" height="18" x="3" y="3" rx="2"/>',
    'download': '<path d="M12 15V3"/><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"/><path d="m7 10 5 5 5-5"/>',
    'pause': '<rect x="14" y="3" width="5" height="18" rx="1"/><rect x="5" y="3" width="5" height="18" rx="1"/>',
    'trash': '<path d="M10 11v6"/><path d="M14 11v6"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6"/><path d="M3 6h18"/><path d="M8 6V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/>',
    'info': '<circle cx="12" cy="12" r="10"/><path d="M12 16v-4"/><path d="M12 8h.01"/>',
    'alert': '<circle cx="12" cy="12" r="10"/><line x1="12" x2="12" y1="8" y2="12"/><line x1="12" x2="12.01" y1="16" y2="16"/>',
    'languages': '<path d="m5 8 6 6"/><path d="m4 14 6-6 2-3"/><path d="M2 5h12"/><path d="M7 2h1"/><path d="m22 22-5-10-5 10"/><path d="M14 18h6"/>',
    'plus': '<path d="M5 12h14"/><path d="M12 5v14"/>',
  };
  const sprite = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
  sprite.setAttribute('style', 'display:none');
  sprite.innerHTML = Object.entries(G)
    .map(([k, v]) => `<symbol id="i-${k}" viewBox="0 0 24 24">${v}</symbol>`).join('');
  document.addEventListener('DOMContentLoaded', () => {
    document.body.prepend(sprite);
    const root = document.documentElement;
    let theme = 'dark', os = 'linux';
    try {
      theme = localStorage.getItem('paraphrase-mock-theme') || theme;
      os = localStorage.getItem('paraphrase-mock-os') || os;
    } catch (e) {}
    const q = new URLSearchParams(location.search);
    if (q.get('theme')) theme = q.get('theme');
    if (q.get('os')) os = q.get('os');
    root.setAttribute('data-theme', theme);
    root.setAttribute('data-os', os);
    const tb = document.createElement('div');
    tb.id = 'review';
    const name = document.title.replace(/^Paraphrase mock: /, '');
    tb.innerHTML = `<a href="index.html">Index</a><span class="name">${name}</span>
      <button id="rv-notes">Notes</button>
      <button id="rv-theme">${theme === 'dark' ? 'Dark' : 'Light'}</button>
      <select id="rv-os"><option value="linux">Linux</option><option value="windows">Windows</option><option value="macos">macOS</option></select>`;
    document.body.append(tb);
    tb.querySelector('#rv-os').value = os;
    tb.querySelector('#rv-theme').onclick = () => {
      theme = theme === 'dark' ? 'light' : 'dark';
      root.setAttribute('data-theme', theme);
      tb.querySelector('#rv-theme').textContent = theme === 'dark' ? 'Dark' : 'Light';
      try { localStorage.setItem('paraphrase-mock-theme', theme); } catch (e) {}
    };
    tb.querySelector('#rv-os').onchange = (e) => {
      os = e.target.value;
      root.setAttribute('data-os', os);
      try { localStorage.setItem('paraphrase-mock-os', os); } catch (e2) {}
    };
    // Notes are the yellow review annotations; ?notes=0 hides them (for screenshots).
    if (q.get('notes') === '0') document.body.classList.add('clean');
    tb.querySelector('#rv-notes').onclick = () => document.body.classList.toggle('clean');
    if (q.get('review') === '0') tb.remove();
  });
})();
