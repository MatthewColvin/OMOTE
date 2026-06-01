/* OMOTE firmware config editor — beginner-first, auto-linked JSON files */
const SCR_W = 240;
const SCR_H = 320;
const STATUS_H = 22;
const TAB_BAR_H = Math.round(SCR_H * 0.1);
const CONTENT_H = SCR_H - STATUS_H - TAB_BAR_H;
const ADVANCED_KEY = 'omote_editor_advanced';
const OMOTE_PACK_VERSION = 1;
const HA_SETTINGS_PATH = 'HaSettings.json';
const HA_DOMAINS = ['light', 'switch', 'cover', 'climate', 'sensor', 'media_player', 'fan', 'scene', 'script', 'input_boolean', 'lock', 'button'];
const HA_EDITOR_PREFS_KEY = 'omote_oo_ha_editor_prefs';

/** Live HA state for canvas preview (browser only). */
const haStateCache = new Map();
let haPreviewTimer = null;
let haActiveDomain = 'light';

/** Match LVGL++ widget constants (NumberPad.hpp, ColorButtons.hpp, JsonPage distBetweenWidgets). */
const FW_LAYOUT = {
  gap: 5,
  padX: 8,
  colorButtons: { height: 25, btnW: 40, spacingX: 20, marginX: 10 },
  numberPad: { height: 180, btnW: 60, btnH: 30, spacingX: 20, spacingY: 15, pad: 10, cols: 3 },
};

const DRAGGABLE_WIDGET_TYPES = new Set(['Button', 'Title', 'Label', 'Image', 'HaToggle', 'HaLabel']);

const NUM_PAD_COMMANDS = [
  'NUM_0', 'NUM_1', 'NUM_2', 'NUM_3', 'NUM_4',
  'NUM_5', 'NUM_6', 'NUM_7', 'NUM_8', 'NUM_9'
];

/** Firmware JsonPage widget types and defaults. */
const WIDGET_TYPES = {
  Button: {
    label: 'Button',
    hint: 'Tappable control — sends an IR or other command when pressed.',
    create(widgets) {
      return { Type: 'Button', Text: 'New button', Command: '', HeightPct: 10, AlignTo: widgets.length };
    }
  },
  Title: {
    label: 'Title',
    hint: 'Heading bar — shows the device tab name at the top of the page.',
    create(widgets) {
      return { Type: 'Title', HeightPct: 10, AlignTo: widgets.length };
    }
  },
  Label: {
    label: 'Label',
    hint: 'Text line — static label or MQTT-bound status (Advanced).',
    create(widgets) {
      return { Type: 'Label', Text: 'Status', HeightPct: 8, AlignTo: widgets.length };
    }
  },
  Image: {
    label: 'Image',
    hint: 'PNG from the Images/ folder on the remote.',
    create(widgets) {
      return {
        Type: 'Image',
        FileName: 'Images/OMOTE_Logo.png',
        SizeXYinPixels: [120, 120],
        AlignTo: widgets.length
      };
    }
  },
  ColorButtons: {
    label: 'Color buttons',
    hint: 'Red / green / yellow / blue row (fixed layout).',
    create(widgets) {
      return {
        Type: 'ColorButtons',
        Command: ['RED', 'GREEN', 'YELLOW', 'BLUE'],
        AlignTo: widgets.length
      };
    },
    stubCommands: ['RED', 'GREEN', 'YELLOW', 'BLUE']
  },
  NumberPad: {
    label: 'Number pad',
    hint: '0–9 dial pad grid (fixed layout).',
    create(widgets) {
      return { Type: 'NumberPad', Command: [...NUM_PAD_COMMANDS], AlignTo: widgets.length };
    },
    stubCommands: NUM_PAD_COMMANDS
  },
  HaToggle: {
    label: 'HA toggle',
    hint: 'Home Assistant toggle button (light, switch, etc.).',
    create() {
      return { Type: 'HaToggle', Text: 'Device', EntityId: '', Domain: 'light', Service: 'toggle', HeightPct: 10, AlignTo: 0 };
    }
  },
  HaLabel: {
    label: 'HA label',
    hint: 'Shows live entity state from Home Assistant.',
    create() {
      return { Type: 'HaLabel', Text: '—', EntityId: '', HeightPct: 8, AlignTo: 0 };
    }
  }
};

const KEY_LABELS = {
  Power: 'Power', Stop: 'Stop', Rewind: 'Rewind', Play: 'Play', FastForward: 'Forward',
  Menu: 'Menu', Info: 'Info', Back: 'Back', Source: 'Source',
  Up: 'Up', Down: 'Down', Left: 'Left', Right: 'Right', Center: 'OK',
  VolUp: 'Vol+', VolDown: 'Vol-', Mute: 'Mute', Record: 'Record',
  ChannelUp: 'CH+', ChannelDown: 'CH-',
  Aux1: 'Red', Aux2: 'Green', Aux3: 'Yellow', Aux4: 'Blue'
};

const DEFAULT_CMD_FOR_KEY = {
  Up: 'UP', Down: 'DOWN', Left: 'LEFT', Right: 'RIGHT', Center: 'SELECT',
  VolUp: 'VOL_UP', VolDown: 'VOL_DOWN', Mute: 'MUTE',
  ChannelUp: 'CHAN_UP', ChannelDown: 'CHAN_DOWN',
  Power: 'PWR_TOGGLE', Back: 'BACK', Menu: 'MENU', Info: 'INFO', Source: 'SOURCE',
  Stop: 'STOP', Play: 'PLAY', Rewind: 'REWIND', FastForward: 'FORWARD',
  Aux1: 'RED', Aux2: 'GREEN', Aux3: 'YELLOW', Aux4: 'BLUE'
};

const DEVICE_TEMPLATES = {
  blank: {
    label: 'Blank — learn everything yourself',
    page: { Widgets: [], ButtonMaps: {} },
    commands: { Manufacturer: 'Custom', DeviceClass: 'Generic', Commands: [] }
  },
  tv: {
    label: 'TV (buttons + color keys + number pad)',
    page: {
      Widgets: [
        { Type: 'Button', Text: 'Source', Command: 'SOURCE', HeightPct: 10, AlignTo: 0 },
        { Type: 'ColorButtons', Command: ['RED', 'GREEN', 'YELLOW', 'BLUE'], AlignTo: 1 },
        { Type: 'NumberPad', Command: ['NUM_0','NUM_1','NUM_2','NUM_3','NUM_4','NUM_5','NUM_6','NUM_7','NUM_8','NUM_9'], AlignTo: 2 }
      ],
      ButtonMaps: {
        Up: { Press: 'UP' }, Down: { Press: 'DOWN' }, Left: { Press: 'LEFT' }, Right: { Press: 'RIGHT' },
        Center: { Press: 'SELECT' }, VolUp: { Press: 'VOL_UP', Repeat: 'VOL_UP' },
        VolDown: { Press: 'VOL_DOWN', Repeat: 'VOL_DOWN' }, Mute: { Press: 'MUTE' },
        ChannelUp: { Press: 'CHAN_UP' }, ChannelDown: { Press: 'CHAN_DOWN' },
        Back: { Press: 'BACK' }, Menu: { Press: 'MENU' }, Info: { Press: 'INFO' }
      }
    },
    commands: { Manufacturer: 'Custom', DeviceClass: 'TV', Commands: [] }
  },
  roku: {
    label: 'Streaming (Roku-style shortcuts)',
    pageFile: 'Pages/Page_Roku.json',
    cmdFile: 'Commands/Commands_Roku.json'
  },
  avreceiver: {
    label: 'AV receiver',
    pageFile: 'Pages/Page_AVReceiver.json',
    cmdFile: 'Commands/Commands_PanasonicTV.json'
  }
};

let API = localStorage.getItem('omote_oo_api') || 'http://omote.local';
let advancedMode = localStorage.getItem(ADVANCED_KEY) === '1';
const files = new Map();
let selectedScenePath = '';
let activeTabIdx = 0;
let selectedPagePath = '';
let selectedWidgetIdx = -1;
let selectedKeyName = '';
let selectedCmdFile = '';
let selectedRawFile = '';
let selection = { kind: null, widgetIdx: null, keyName: null };
let drag = null;
let canvasScrollY = 0;
let lastPreviewPagePath = '';

const $ = (id) => document.getElementById(id);

function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

/** Firmware JSON allows // and block comments (RapidJSON kParseCommentsFlag). */
function stripJsonComments(text) {
  let out = '';
  let i = 0;
  let inString = false;
  let quote = '';
  while (i < text.length) {
    const c = text[i];
    if (inString) {
      out += c;
      if (c === '\\' && i + 1 < text.length) out += text[++i];
      else if (c === quote) inString = false;
      i++;
      continue;
    }
    if (c === '"' || c === "'") {
      inString = true;
      quote = c;
      out += c;
      i++;
      continue;
    }
    if (c === '/' && i + 1 < text.length) {
      if (text[i + 1] === '/') {
        i += 2;
        while (i < text.length && text[i] !== '\n') i++;
        continue;
      }
      if (text[i + 1] === '*') {
        i += 2;
        while (i + 1 < text.length && !(text[i] === '*' && text[i + 1] === '/')) i++;
        i += 2;
        continue;
      }
    }
    out += c;
    i++;
  }
  return out;
}

function parseJsonText(raw) {
  if (!raw) return null;
  try {
    return JSON.parse(stripJsonComments(raw));
  } catch {
    try { return JSON.parse(raw); } catch { return null; }
  }
}

function slugify(s) {
  return (s || 'device').replace(/[^a-z0-9]+/gi, '_').replace(/^_|_$/g, '') || 'Device';
}

function defaultApi() {
  const o = window.location.origin;
  if (o && o.startsWith('http') && !o.includes('localhost') && !o.includes('127.0.0.1'))
    return o.replace(/\/$/, '');
  return API;
}

async function api(path, opts = {}) {
  const base = API.replace(/\/$/, '');
  const ctrl = new AbortController();
  const t = setTimeout(() => ctrl.abort(), opts.timeout || 15000);
  try {
    const res = await fetch(base + path, { ...opts, signal: ctrl.signal });
    clearTimeout(t);
    if (!res.ok) throw new Error(res.status + ' ' + await res.text());
    const ct = res.headers.get('content-type') || '';
    return ct.includes('json') ? res.json() : res.text();
  } catch (e) {
    clearTimeout(t);
    throw e;
  }
}

function parseJson(path) {
  const raw = files.get(path)?.content;
  return parseJsonText(raw);
}

function setFile(path, content, dirty = true) {
  files.set(path, {
    content: typeof content === 'string' ? content : JSON.stringify(content, null, 2),
    dirty
  });
}

function listPaths(prefix) {
  return [...files.keys()].filter((p) => p.startsWith(prefix)).sort();
}

function normalizePackPath(path) {
  return path.replace(/\\/g, '/').replace(/^\/+/, '');
}

function isPackConfigPath(path) {
  const p = normalizePackPath(path);
  if (!p || p.endsWith('/')) return false;
  if (p === 'manifest.json') return false;
  if (p.startsWith('__MACOSX/') || p.includes('/__MACOSX/')) return false;
  if (p.endsWith('.DS_Store')) return false;
  return p.endsWith('.json');
}

function packFileEntries() {
  return [...files.entries()]
    .map(([path, entry]) => [normalizePackPath(path), entry.content])
    .filter(([path]) => isPackConfigPath(path))
    .sort((a, b) => a[0].localeCompare(b[0]));
}

function setConnectMsg(text, kind = '') {
  const el = $('connect-msg');
  if (!el) return;
  el.textContent = text;
  el.className = 'msg' + (kind ? ' ' + kind : '');
}

function defaultHaService(domain, widgetType) {
  const d = domain || 'light';
  if (widgetType === 'HaLabel') return 'turn_on';
  if (d === 'scene' || d === 'script') return 'turn_on';
  if (d === 'button') return 'press';
  return 'toggle';
}

function isHaStateOn(state) {
  const s = String(state || '').toLowerCase();
  return ['on', 'true', '1', 'yes', 'open', 'opening', 'playing', 'home', 'heat', 'cool', 'auto', 'unlocked', 'active'].includes(s);
}

function loadHaSettingsDoc() {
  return parseJson(HA_SETTINGS_PATH) || { Url: '', Token: '' };
}

function loadHaSettingsForm() {
  const doc = loadHaSettingsDoc();
  const prefs = JSON.parse(localStorage.getItem(HA_EDITOR_PREFS_KEY) || '{}');
  if ($('ha-url')) $('ha-url').value = doc.Url || prefs.ha_url || '';
  if ($('ha-token')) $('ha-token').value = doc.Token || prefs.ha_token || '';
}

function saveHaSettingsToFiles() {
  const url = ($('ha-url')?.value || '').trim().replace(/\/+$/, '');
  const token = ($('ha-token')?.value || '').trim();
  setFile(HA_SETTINGS_PATH, { Url: url, Token: token });
  localStorage.setItem(HA_EDITOR_PREFS_KEY, JSON.stringify({ ha_url: url, ha_token: token }));
}

function getHaCredentials() {
  const url = ($('ha-url')?.value || '').trim().replace(/\/+$/, '');
  const token = ($('ha-token')?.value || '').trim();
  if (!url || !token) return null;
  return { url, token };
}

async function haBrowserFetch(path, opts = {}) {
  const creds = getHaCredentials();
  if (!creds) throw new Error('Enter HA URL and token on the Connect tab.');
  const timeoutMs = opts.timeoutMs || 8000;
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), timeoutMs);
  try {
    const res = await fetch(`${creds.url}${path}`, {
      method: opts.method || 'GET',
      body: opts.body,
      signal: controller.signal,
      headers: {
        Authorization: `Bearer ${creds.token}`,
        'Content-Type': 'application/json',
        ...(opts.headers || {}),
      },
    });
    const text = await res.text();
    if (!res.ok) {
      let err = text;
      try {
        const j = JSON.parse(text);
        err = j.message || j.error || text;
      } catch { /* ignore */ }
      throw new Error(typeof err === 'string' ? err : res.statusText);
    }
    return text;
  } finally {
    clearTimeout(timer);
  }
}

function haBrowserErrorHint(err) {
  const m = String(err?.message || err || '');
  if (m === 'Failed to fetch' || err?.name === 'TypeError') {
    return 'Browser cannot reach Home Assistant (CORS or wrong URL). Add this editor to HA http.cors_allowed_origins.';
  }
  if (m.includes('401') || m.toLowerCase().includes('unauthorized')) {
    return 'HA rejected the token — create a new long-lived access token.';
  }
  return m;
}

async function haBrowserListEntities(domain, search) {
  const dom = domain || 'light';
  const prefix = dom + '.';
  const text = await haBrowserFetch('/api/states', { timeoutMs: 20000 });
  const states = JSON.parse(text);
  if (!Array.isArray(states)) throw new Error('Unexpected /api/states response');
  const q = (search || '').toLowerCase();
  const entities = [];
  for (const s of states) {
    const entity_id = s.entity_id || '';
    if (!entity_id.startsWith(prefix)) continue;
    const friendly_name = s.attributes?.friendly_name || '';
    if (q) {
      const blob = `${entity_id} ${friendly_name}`.toLowerCase();
      if (!blob.includes(q)) continue;
    }
    entities.push({
      entity_id,
      state: s.state || '',
      friendly_name,
      domain: dom,
    });
    if (entities.length >= 120) break;
  }
  return { entities };
}

async function haBrowserEntityState(entityId) {
  const text = await haBrowserFetch(`/api/states/${encodeURIComponent(entityId)}`);
  const s = JSON.parse(text);
  return { entity_id: entityId, state: s.state || '', friendly_name: s.attributes?.friendly_name || '' };
}

function renderHaDomainTabs() {
  const root = $('ha-domain-tabs');
  if (!root) return;
  root.innerHTML = '';
  HA_DOMAINS.forEach((d) => {
    const b = document.createElement('button');
    b.type = 'button';
    b.textContent = d;
    b.className = d === haActiveDomain ? 'active' : '';
    b.onclick = () => {
      haActiveDomain = d;
      renderHaDomainTabs();
      populateHaEntityPicker({ domain: d });
    };
    root.appendChild(b);
  });
}

async function populateHaEntityPicker(opts = {}) {
  const status = $('ha-entity-status');
  const sel = $('ha-entity-pick');
  if (!status || !sel) return;
  const domain = opts.domain || haActiveDomain || 'light';
  const search = ($('ha-entity-search')?.value || '').trim();
  const selected = opts.selected || sel.value || '';
  status.textContent = 'Loading entities…';
  sel.innerHTML = '';
  try {
    const data = await haBrowserListEntities(domain, search);
    status.textContent = `${data.entities.length} ${domain} entities`;
    data.entities.forEach((e) => {
      const o = document.createElement('option');
      o.value = e.entity_id;
      o.textContent = `${e.friendly_name || e.entity_id} — ${e.state}`;
      if (e.entity_id === selected) o.selected = true;
      sel.appendChild(o);
    });
    data.entities.forEach((e) => haStateCache.set(e.entity_id, e.state));
    if (opts.autoApplyLabel !== false) applyHaEntityPickToWidget();
    drawCanvas();
  } catch (e) {
    status.textContent = haBrowserErrorHint(e);
  }
}

function applyHaEntityPickToWidget() {
  if (selection.kind !== 'widget') return;
  const page = currentPage();
  const w = page.Widgets?.[selection.widgetIdx];
  if (!w || (w.Type !== 'HaToggle' && w.Type !== 'HaLabel')) return;
  const sel = $('ha-entity-pick');
  const opt = sel?.selectedOptions?.[0];
  if (!opt?.value) return;
  w.EntityId = opt.value;
  w.Domain = opt.value.split('.')[0] || 'light';
  if (w.Type === 'HaToggle') {
    w.Service = $('ha-service')?.value || defaultHaService(w.Domain, w.Type);
  }
  if ($('ha-auto-label')?.checked) {
    const label = opt.textContent.split(' — ')[0] || opt.value;
    w.Text = label;
    if ($('action-touch-label')) $('action-touch-label').value = label;
  }
  savePage(page);
  drawCanvas();
}

function pageHaEntityIds(page) {
  const ids = new Set();
  (page?.Widgets || []).forEach((w) => {
    if (w.EntityId) ids.add(w.EntityId);
  });
  return [...ids];
}

async function refreshHaPreviewStates() {
  const creds = getHaCredentials();
  if (!creds) return;
  const ids = pageHaEntityIds(currentPage());
  if (!ids.length) return;
  let changed = false;
  for (const eid of ids.slice(0, 12)) {
    try {
      const data = await haBrowserEntityState(eid);
      const prev = haStateCache.get(eid);
      if (prev !== data.state) {
        haStateCache.set(eid, data.state);
        changed = true;
      }
    } catch { /* skip */ }
  }
  if (changed) drawCanvas();
}

function startHaPreviewPolling() {
  stopHaPreviewPolling();
  haPreviewTimer = setInterval(() => refreshHaPreviewStates().catch(() => {}), 8000);
}

function stopHaPreviewPolling() {
  if (haPreviewTimer) {
    clearInterval(haPreviewTimer);
    haPreviewTimer = null;
  }
}

function localFileMap() {
  const m = new Map();
  for (const [path, { content }] of files.entries()) {
    if (isPackConfigPath(path)) m.set(normalizePackPath(path), content);
  }
  return m;
}

function normalizeContentForCompare(content) {
  const parsed = parseJsonText(content);
  if (parsed !== null) return JSON.stringify(parsed);
  return (content || '').trim();
}

function diffEditorVsRemote(localMap, remoteMap) {
  const onlyLocal = [];
  const onlyRemote = [];
  const changed = [];
  const paths = new Set([...localMap.keys(), ...remoteMap.keys()]);
  for (const p of [...paths].sort()) {
    const local = localMap.get(p);
    const remote = remoteMap.get(p);
    if (local === undefined) {
      onlyRemote.push(p);
      continue;
    }
    if (remote === undefined) {
      onlyLocal.push(p);
      continue;
    }
    if (normalizeContentForCompare(local) !== normalizeContentForCompare(remote)) changed.push(p);
  }
  return { onlyLocal, onlyRemote, changed, hasDiff: !!(onlyLocal.length || onlyRemote.length || changed.length) };
}

async function fetchRemoteFileMap(tree) {
  const remote = new Map();
  for (const p of tree.files || []) {
    if (!isPackConfigPath(p)) continue;
    const r = await api('/api/fs/read?path=' + encodeURIComponent(p));
    remote.set(normalizePackPath(p), r.content);
  }
  return remote;
}

async function loadRemoteIntoEditor(tree) {
  files.clear();
  for (const p of tree.files || []) {
    const r = await api('/api/fs/read?path=' + encodeURIComponent(p));
    setFile(p, r.content, false);
  }
  if (!files.has('Scenes.json')) setFile('Scenes.json', { Scenes: [] }, false);
  syncOrphanSceneFiles();
  initAfterLoad();
}

function formatStatusBar(st, note = '') {
  const base = `${st.connected ? 'Connected' : 'Offline'} · ${st.ip || '?'} · ${st.hostname}.local${st.editor_sync ? ' · sync' : ''}`;
  return note ? `${base}${note}` : base;
}

function unsavedFileCount() {
  return [...files.values()].filter((v) => v.dirty).length;
}

function summarizeConfigDiff(diff) {
  const parts = [];
  if (diff.changed.length) parts.push(`${diff.changed.length} changed`);
  if (diff.onlyLocal.length) parts.push(`${diff.onlyLocal.length} only in editor`);
  if (diff.onlyRemote.length) parts.push(`${diff.onlyRemote.length} only on remote`);
  return parts.join(' · ') || 'differences found';
}

function diffPreviewLines(diff, limit = 10) {
  const lines = [];
  diff.changed.forEach((p) => lines.push({ kind: 'changed', path: p }));
  diff.onlyLocal.forEach((p) => lines.push({ kind: 'local', path: p }));
  diff.onlyRemote.forEach((p) => lines.push({ kind: 'remote', path: p }));
  return lines.slice(0, limit);
}

function askConnectConflictChoice(diff) {
  return new Promise((resolve) => {
    const modal = $('connect-conflict-modal');
    const summary = $('connect-conflict-summary');
    const list = $('connect-conflict-list');
    if (!modal || !summary || !list) {
      resolve('keep');
      return;
    }

    const dirty = unsavedFileCount();
    summary.textContent = `${summarizeConfigDiff(diff)}${dirty ? ` · ${dirty} unsaved in editor` : ''}.`;

    list.innerHTML = '';
    const preview = diffPreviewLines(diff, 12);
    preview.forEach(({ kind, path }) => {
      const li = document.createElement('li');
      const tag = kind === 'changed' ? 'changed' : kind === 'local' ? 'editor only' : 'remote only';
      li.textContent = `${path} (${tag})`;
      list.appendChild(li);
    });
    const total = diff.changed.length + diff.onlyLocal.length + diff.onlyRemote.length;
    if (total > preview.length) {
      const li = document.createElement('li');
      li.textContent = `…and ${total - preview.length} more`;
      list.appendChild(li);
    }

    modal.classList.remove('hidden');

    const finish = (choice) => {
      modal.classList.add('hidden');
      $('btn-conflict-keep').onclick = null;
      $('btn-conflict-remote').onclick = null;
      $('btn-conflict-cancel').onclick = null;
      resolve(choice);
    };

    $('btn-conflict-keep').onclick = () => finish('keep');
    $('btn-conflict-remote').onclick = () => finish('remote');
    $('btn-conflict-cancel').onclick = () => finish('cancel');
  });
}

async function exportOmotePack() {
  if (typeof JSZip === 'undefined') throw new Error('JSZip not loaded — refresh the page.');
  const entries = packFileEntries();
  if (!entries.length) throw new Error('Nothing to export — connect to a remote or import a backup first.');

  const zip = new JSZip();
  zip.file('manifest.json', JSON.stringify({
    omote_pack_version: OMOTE_PACK_VERSION,
    format: 'omote-config-pack',
    exported_at: new Date().toISOString(),
    source_api: API || null,
    file_count: entries.length
  }, null, 2));

  entries.forEach(([path, content]) => zip.file(path, content));

  const blob = await zip.generateAsync({ type: 'blob', compression: 'DEFLATE' });
  const stamp = new Date().toISOString().slice(0, 10);
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = `omote-config-${stamp}.omote`;
  a.click();
  URL.revokeObjectURL(a.href);
  return entries.length;
}

async function importOmotePack(file) {
  if (typeof JSZip === 'undefined') throw new Error('JSZip not loaded — refresh the page.');
  if (!file) return 0;

  const zip = await JSZip.loadAsync(await file.arrayBuffer());
  const paths = Object.keys(zip.files)
    .filter((path) => isPackConfigPath(path))
    .sort();

  if (!paths.length) throw new Error('No JSON config files found in this .omote archive.');

  const dirtyCount = [...files.values()].filter((v) => v.dirty).length;
  if (files.size && (dirtyCount || paths.length)) {
    const ok = confirm('Replace the current editor files with this backup? Unsaved changes will be lost.');
    if (!ok) return 0;
  }

  files.clear();
  for (const path of paths) {
    const content = await zip.file(path).async('string');
    setFile(normalizePackPath(path), content, true);
  }

  if (!files.has('Scenes.json')) setFile('Scenes.json', { Scenes: [] }, true);
  syncOrphanSceneFiles();
  initAfterLoad();
  $('status-bar').textContent = `Offline · ${files.size} files from backup`;
  showTab('scenes');
  return paths.length;
}

async function handleExportOmotePack() {
  setConnectMsg('Creating backup…');
  try {
    const count = await exportOmotePack();
    setConnectMsg(`Exported ${count} file(s) to .omote backup.`, 'ok');
    $('deploy-msg').textContent = `Exported ${count} file(s).`;
    $('deploy-msg').className = 'msg ok';
  } catch (e) {
    setConnectMsg(e.message, 'err');
    $('deploy-msg').textContent = e.message;
    $('deploy-msg').className = 'msg err';
  }
}

function resolvePagePath(fileName) {
  if (!fileName) return '';
  if (files.has(fileName)) return fileName;
  const inPages = 'Pages/' + fileName.replace(/^Pages\//, '');
  if (files.has(inPages)) return inPages;
  return fileName;
}

/** Scene tabs created by older editor builds omitted the Pages/ prefix — fix on load. */
function normalizeScenePagePaths() {
  let fixed = false;
  for (const path of listPaths('Scenes/')) {
    const sc = parseJson(path);
    if (!sc?.Pages?.length) continue;
    let changed = false;
    for (const pg of sc.Pages) {
      if (!pg.FileName) continue;
      const resolved = resolvePagePath(pg.FileName);
      if (resolved && resolved !== pg.FileName && resolved.startsWith('Pages/')) {
        pg.FileName = resolved;
        changed = true;
      }
    }
    if (changed) {
      setFile(path, sc);
      fixed = true;
    }
  }
  return fixed;
}

function sceneContext() {
  const scene = parseJson(selectedScenePath);
  const entry = scene?.Pages?.[activeTabIdx];
  const pagePath = entry ? resolvePagePath(entry.FileName) : selectedPagePath;
  return { scene, entry, pagePath, commandPrefix: entry?.CommandPrefix || '' };
}

function sceneRegistry() {
  if (!files.has('Scenes.json')) return { Scenes: [] };
  return parseJson('Scenes.json') || { Scenes: [] };
}

function setSceneRegistry(reg) {
  setFile('Scenes.json', reg);
}

function sceneRegistryEntry() {
  return sceneRegistry().Scenes?.find((s) => s.FileName === selectedScenePath) || null;
}

function syncOrphanSceneFiles() {
  const reg = sceneRegistry();
  reg.Scenes = reg.Scenes || [];
  const registered = new Set(reg.Scenes.map((s) => s.FileName));
  let changed = false;
  for (const path of listPaths('Scenes/')) {
    if (registered.has(path)) continue;
    const sc = parseJson(path);
    const base = path.replace(/^Scenes\/Scene_/, '').replace(/\.json$/, '');
    const name = sc?.ScreenName || base.replace(/_/g, ' ');
    reg.Scenes.push({ SceneName: name, FileName: path });
    changed = true;
  }
  if (changed) setSceneRegistry(reg);
}

function findSceneForPage(pagePath) {
  for (const sp of listPaths('Scenes/')) {
    const sc = parseJson(sp);
    if (!sc?.Pages) continue;
    for (let i = 0; i < sc.Pages.length; i++) {
      if (resolvePagePath(sc.Pages[i].FileName) === pagePath)
        return { scenePath: sp, tabIdx: i };
    }
  }
  return null;
}

/* ── Auto-link: create page + command file for a device ── */
function ensureCommandFile(slug, templateKey) {
  const path = `Commands/Commands_${slug}.json`;
  if (files.has(path)) return path;
  const tpl = DEVICE_TEMPLATES[templateKey];
  if (tpl?.cmdFile && files.has(tpl.cmdFile)) {
    setFile(path, parseJson(tpl.cmdFile));
    return path;
  }
  setFile(path, JSON.parse(JSON.stringify(tpl?.commands || DEVICE_TEMPLATES.blank.commands)));
  return path;
}

function ensurePageFile(slug, templateKey, cmdPath) {
  const path = `Pages/Page_${slug}.json`;
  if (files.has(path)) {
    const pg = parseJson(path);
    if (!pg.CommandFile) { pg.CommandFile = cmdPath; setFile(path, pg); }
    return path;
  }
  const tpl = DEVICE_TEMPLATES[templateKey];
  if (tpl?.pageFile && files.has(tpl.pageFile)) {
    const pg = parseJson(tpl.pageFile);
    pg.CommandFile = cmdPath;
    setFile(path, pg);
    return path;
  }
  const pg = JSON.parse(JSON.stringify(tpl?.page || DEVICE_TEMPLATES.blank.page));
  pg.CommandFile = cmdPath;
  setFile(path, pg);
  return path;
}

function addDeviceToScene(pageName, templateKey = 'blank', shortName) {
  const slug = slugify(pageName);
  const cmdPath = ensureCommandFile(slug, templateKey);
  const pagePath = ensurePageFile(slug, templateKey, cmdPath);
  const scene = parseJson(selectedScenePath) || { Type: 'Scene', Pages: [] };
  scene.Pages = scene.Pages || [];
  scene.Pages.push({
    PageName: pageName,
    ShortName: (shortName || pageName).slice(0, 12),
    FileName: pagePath
  });
  if (!scene.ScreenName) scene.ScreenName = $('scene-screen-name')?.value || 'My scene';
  setFile(selectedScenePath, scene);
  activeTabIdx = scene.Pages.length - 1;
  selectedPagePath = pagePath;
  refreshAll();
}

function promptNewDeviceTab() {
  const pageName = prompt('Device name (shown at top of this tab):', 'Living room TV');
  if (!pageName?.trim()) return null;
  const tabLabel = prompt(
    'Tab label (bottom bar — keep short, e.g. BD, TV, Amp):',
    pageName.trim().slice(0, 8)
  );
  if (tabLabel === null) return null;
  return {
    pageName: pageName.trim(),
    shortName: (tabLabel.trim() || pageName.trim()).slice(0, 12)
  };
}

function saveScenePageTab(idx, pageName, shortName) {
  const scene = parseJson(selectedScenePath);
  if (!scene?.Pages?.[idx]) return;
  scene.Pages[idx].PageName = pageName;
  scene.Pages[idx].ShortName = (shortName || pageName).slice(0, 12);
  setFile(selectedScenePath, scene);
  if (idx === activeTabIdx) {
    $('remote-device-label').textContent = scene.Pages[idx].PageName || scene.Pages[idx].ShortName || 'Device';
    const regEntry = sceneRegistryEntry();
    const sceneLabel = regEntry?.SceneName || scene.ScreenName || 'Scene';
    const hint = $('editing-hint');
    if (hint) {
      hint.textContent = `Scene: ${sceneLabel} · “${scene.Pages[idx].ShortName || scene.Pages[idx].PageName}” — edit screen & keys on the remote.`;
    }
  }
  drawCanvas();
}

function removeScenePageTab(idx) {
  const scene = parseJson(selectedScenePath);
  if (!scene?.Pages?.[idx]) return;
  const pg = scene.Pages[idx];
  const label = pg.ShortName || pg.PageName || `Tab ${idx + 1}`;
  if (!confirm(`Remove tab “${label}” from this scene?\n\nThe page file (${pg.FileName}) stays on the remote.`))
    return;
  scene.Pages.splice(idx, 1);
  setFile(selectedScenePath, scene);
  activeTabIdx = Math.min(activeTabIdx, Math.max(0, scene.Pages.length - 1));
  if (scene.Pages[activeTabIdx]) {
    selectedPagePath = resolvePagePath(scene.Pages[activeTabIdx].FileName);
  } else {
    selectedPagePath = '';
    clearSelection();
  }
  refreshAll();
}

function renderDeviceTabEditor(container, scene, mode = 'scenes') {
  if (!container) return;
  container.innerHTML = '';
  container.classList.add('device-tab-list');
  const pages = scene?.Pages || [];
  if (!pages.length) {
    const empty = document.createElement('div');
    empty.className = 'device-tab-empty';
    empty.textContent = scene
      ? 'No tabs yet — add one below.'
      : 'Could not load tabs from this scene.';
    container.appendChild(empty);
    return;
  }

  pages.forEach((pg, idx) => {
    const card = document.createElement('div');
    card.className = 'device-tab-card';
    if (mode === 'remote' && idx === activeTabIdx) card.classList.add('is-active');

    const head = document.createElement('div');
    head.className = 'device-tab-card-head';

    const badge = document.createElement('span');
    badge.className = 'tab-badge';
    badge.textContent = (pg.ShortName || pg.PageName || '?').slice(0, 8);

    const title = document.createElement('span');
    title.className = 'device-tab-card-title';
    title.textContent = pg.PageName || pg.ShortName || `Tab ${idx + 1}`;

    const actions = document.createElement('div');
    actions.className = 'device-tab-card-actions';

    const updateHead = () => {
      badge.textContent = (tabIn.value || nameIn.value || '?').slice(0, 8);
      title.textContent = nameIn.value || tabIn.value || `Tab ${idx + 1}`;
    };

    const del = document.createElement('button');
    del.type = 'button';
    del.className = 'icon-btn danger';
    del.textContent = '×';
    del.title = 'Remove tab';
    del.setAttribute('aria-label', 'Remove tab');
    del.onclick = (e) => {
      e.stopPropagation();
      removeScenePageTab(idx);
    };
    actions.appendChild(del);

    if (mode === 'scenes') {
      const configure = document.createElement('button');
      configure.type = 'button';
      configure.className = 'icon-btn primary-soft';
      configure.textContent = 'Edit';
      configure.title = 'Configure remote layout';
      configure.onclick = (e) => {
        e.stopPropagation();
        activeTabIdx = idx;
        selectedPagePath = resolvePagePath(pg.FileName);
        showTab('remote');
      };
      actions.insertBefore(configure, del);
    }

    head.append(badge, title, actions);

    const fields = document.createElement('div');
    fields.className = 'device-tab-fields';

    const tabField = document.createElement('div');
    tabField.className = 'field-mini';
    tabField.innerHTML = '<label>Tab label</label>';
    const tabIn = document.createElement('input');
    tabIn.value = pg.ShortName || pg.PageName || '';
    tabIn.placeholder = 'BD, TV, Amp…';
    tabIn.maxLength = 12;
    tabField.appendChild(tabIn);

    const nameField = document.createElement('div');
    nameField.className = 'field-mini';
    nameField.innerHTML = '<label>Device name</label>';
    const nameIn = document.createElement('input');
    nameIn.value = pg.PageName || pg.ShortName || '';
    nameIn.placeholder = 'Living room TV';
    nameField.appendChild(nameIn);

    fields.append(tabField, nameField);

    const commit = () => {
      saveScenePageTab(idx, nameIn.value.trim(), tabIn.value.trim());
      updateHead();
    };
    nameIn.oninput = tabIn.oninput = commit;

    card.append(head, fields);

    if (mode === 'remote') {
      card.style.cursor = 'pointer';
      card.onclick = (e) => {
        if (e.target.closest('input') || e.target.closest('button')) return;
        activeTabIdx = idx;
        selectedPagePath = resolvePagePath(pg.FileName);
        clearSelection();
        refreshRemoteTab();
      };
    }

    container.appendChild(card);
  });
}

function renderDeviceTabList(scene) {
  renderDeviceTabEditor($('device-tab-list'), scene, 'scenes');
}

function addNewScene(name) {
  const slug = slugify(name);
  const scenePath = `Scenes/Scene_${slug}.json`;
  setFile(scenePath, { Type: 'Scene', ScreenName: name, Pages: [] });
  const reg = sceneRegistry();
  reg.Scenes = reg.Scenes || [];
  reg.Scenes.push({ SceneName: name, FileName: scenePath });
  setSceneRegistry(reg);
  selectedScenePath = scenePath;
  activeTabIdx = 0;
  refreshAll();
}

function deleteSelectedScene() {
  if (!selectedScenePath) return;
  const entry = sceneRegistryEntry();
  const label = entry?.SceneName || selectedScenePath;
  if (!confirm(`Remove scene “${label}” from the picker?\n\nThe scene file stays on disk unless you delete it in Advanced → JSON.`))
    return;
  const reg = sceneRegistry();
  reg.Scenes = (reg.Scenes || []).filter((s) => s.FileName !== selectedScenePath);
  setSceneRegistry(reg);
  selectedScenePath = reg.Scenes?.[0]?.FileName || listPaths('Scenes/')[0] || '';
  activeTabIdx = 0;
  refreshAll();
}

function commandFileForPage(pagePath) {
  const pg = parseJson(pagePath);
  return pg?.CommandFile || '';
}

function commandNamesFromFile(path) {
  const d = parseJson(path);
  if (!d) return [];
  return (d.Commands || d.Actions || []).map((c) => c.Command || c.Action).filter(Boolean);
}

function describeCommand(cmdName, pagePath) {
  if (!cmdName) return '';
  const cf = commandFileForPage(pagePath);
  const doc = parseJson(cf);
  const hit = (doc?.Commands || []).find((c) => c.Command === cmdName);
  if (hit?.Data?.[0]) return cmdName + ' · ' + hit.Protocol;
  return cmdName;
}

function getKeyMapping(keyName, pressType = 'Press') {
  const page = currentPage();
  return page.ButtonMaps?.[keyName]?.[pressType] || page.ButtonMaps?.[keyName]?.Press || '';
}

function currentPage() {
  return parseJson(selectedPagePath) || { Widgets: [], CommandFile: '', ButtonMaps: {} };
}

function savePage(page) {
  setFile(selectedPagePath, page);
}

function upsertCommand(cmdFile, name, protocol, code) {
  const doc = parseJson(cmdFile) || { Manufacturer: 'Custom', DeviceClass: 'Generic', Commands: [] };
  doc.Commands = doc.Commands || [];
  const hex = code.startsWith('0x') ? code : '0x' + code;
  let row = doc.Commands.find((c) => c.Command === name);
  if (!row) {
    row = { Command: name, Mode: 'IR', Protocol: protocol, Data: [hex] };
    doc.Commands.push(row);
  } else {
    row.Mode = 'IR';
    row.Protocol = protocol;
    row.Data = [hex];
  }
  setFile(cmdFile, doc);
  return name;
}

function ensurePageCommandFile() {
  const page = currentPage();
  if (page.CommandFile && files.has(page.CommandFile)) return page.CommandFile;
  const slug = slugify($('remote-device-label')?.textContent || 'Device');
  const cf = ensureCommandFile(slug, 'blank');
  page.CommandFile = cf;
  savePage(page);
  return cf;
}

/* ── UI mode ── */
function applyAdvancedMode() {
  document.querySelectorAll('.advanced-field, .nav-advanced').forEach((el) => {
    el.classList.toggle('hidden', !advancedMode);
  });
  $('advanced-mode').checked = advancedMode;
}

function setAdvancedMode(on) {
  advancedMode = !!on;
  localStorage.setItem(ADVANCED_KEY, advancedMode ? '1' : '0');
  applyAdvancedMode();
}

function showTab(name) {
  document.querySelectorAll('.tab').forEach((el) => el.classList.remove('active'));
  document.querySelectorAll('#nav button').forEach((b) => {
    b.classList.toggle('active', b.dataset.tab === name);
  });
  $(`tab-${name}`)?.classList.add('active');
  if (name === 'remote') {
    refreshRemoteTab();
    startHaPreviewPolling();
  } else {
    stopHaPreviewPolling();
  }
  if (name === 'connect') loadHaSettingsForm();
  if (name === 'scenes') refreshScenesTab();
  if (name === 'commands') renderCommandsTable();
  if (name === 'raw') populateRawSelect();
}

document.querySelectorAll('#nav button').forEach((b) => {
  b.onclick = () => showTab(b.dataset.tab);
});

$('advanced-mode').onchange = () => setAdvancedMode($('advanced-mode').checked);
$('device-url').value = defaultApi();
applyAdvancedMode();

async function setEditorSyncMode(on) {
  await api('/api/device/sync-mode', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ on })
  });
}

async function connectAndLoad(options = {}) {
  API = $('device-url').value.trim().replace(/\/$/, '') || 'http://omote.local';
  localStorage.setItem('omote_oo_api', API);
  setConnectMsg('Connecting…');
  try {
    const st = await api('/api/status');
    const tree = await api('/api/fs/tree');
    const hadEditorFiles = files.size > 0;

    if (!hadEditorFiles || options.forceRemote) {
      await loadRemoteIntoEditor(tree);
      $('status-bar').textContent = formatStatusBar(st);
      setConnectMsg(`Loaded ${files.size} files from remote. Start with Scenes to pick or edit “Watch TV”, etc.`, 'ok');
      showTab('scenes');
      return;
    }

    setConnectMsg('Comparing editor with remote…');
    const remoteMap = await fetchRemoteFileMap(tree);
    const diff = diffEditorVsRemote(localFileMap(), remoteMap);

    if (!diff.hasDiff) {
      $('status-bar').textContent = formatStatusBar(st, unsavedFileCount() ? ` · ${unsavedFileCount()} unsaved` : '');
      setConnectMsg(`Connected — editor matches remote (${files.size} files).`, 'ok');
      return;
    }

    const choice = await askConnectConflictChoice(diff);
    if (choice === 'cancel') {
      $('status-bar').textContent = formatStatusBar(st, ` · editor copy${unsavedFileCount() ? ` · ${unsavedFileCount()} unsaved` : ''}`);
      setConnectMsg('Connect cancelled — kept your editor files.', 'ok');
      return;
    }
    if (choice === 'remote') {
      await loadRemoteIntoEditor(tree);
      $('status-bar').textContent = formatStatusBar(st);
      setConnectMsg(`Loaded ${files.size} files from remote (editor copy replaced).`, 'ok');
      showTab('scenes');
      return;
    }

    $('status-bar').textContent = formatStatusBar(st, ` · editor copy${unsavedFileCount() ? ` · ${unsavedFileCount()} unsaved` : ''}`);
    setConnectMsg(`Connected — kept your editor config (${files.size} files). Save to remote when ready.`, 'ok');
  } catch (e) {
    setConnectMsg(e.message, 'err');
  }
}

$('btn-connect').onclick = connectAndLoad;

$('btn-save-ha')?.addEventListener('click', async () => {
  saveHaSettingsToFiles();
  const content = files.get(HA_SETTINGS_PATH)?.content;
  if (!content) {
    setConnectMsg('HA settings saved locally only.', 'ok');
    return;
  }
  setConnectMsg('Saving HA settings…');
  try {
    const st = await api('/api/status');
    if (!st) throw new Error('Not connected to remote');
    await api('/api/fs/write?path=' + encodeURIComponent(HA_SETTINGS_PATH), {
      method: 'POST',
      headers: { 'Content-Type': 'text/plain' },
      body: content,
      timeout: 15000,
    });
    files.get(HA_SETTINGS_PATH).dirty = false;
    setConnectMsg('HA settings saved on remote (LittleFS). Taps work without reboot.', 'ok');
  } catch (e) {
    setConnectMsg(
      `Saved in editor only — use “Save to remote” to push all files. (${e.message})`,
      'ok'
    );
  }
});

$('btn-test-ha')?.addEventListener('click', async () => {
  setConnectMsg('Testing Home Assistant…');
  try {
    await haBrowserFetch('/api/');
    const data = await haBrowserListEntities('light', '');
    setConnectMsg(`Connected to HA — found ${data.entities.length} light entities (sample).`, 'ok');
  } catch (e) {
    setConnectMsg(haBrowserErrorHint(e), 'err');
  }
});

$('ha-entity-search')?.addEventListener('input', () => {
  populateHaEntityPicker({ autoApplyLabel: false }).catch(() => {});
});
$('ha-entity-pick')?.addEventListener('change', () => applyHaEntityPickToWidget());
$('ha-service')?.addEventListener('change', () => applyHaEntityPickToWidget());

$('btn-export-omote')?.addEventListener('click', handleExportOmotePack);
$('btn-export-omote-footer')?.addEventListener('click', handleExportOmotePack);
$('omote-import-file')?.addEventListener('change', async (ev) => {
  const input = ev.target;
  const file = input.files?.[0];
  input.value = '';
  if (!file) return;
  setConnectMsg('Importing backup…');
  try {
    const count = await importOmotePack(file);
    if (!count) return;
    setConnectMsg(`Loaded ${count} file(s) from backup. Edit offline or Save to remote when ready.`, 'ok');
  } catch (e) {
    setConnectMsg('Import failed: ' + e.message, 'err');
  }
});

$('btn-enter-sync').onclick = async () => {
  try {
    API = $('device-url').value.trim().replace(/\/$/, '') || API;
    await setEditorSyncMode(true);
    $('connect-msg').textContent = 'Sync mode enabled on remote.';
    $('connect-msg').className = 'msg ok';
  } catch (e) {
    $('connect-msg').textContent = e.message;
    $('connect-msg').className = 'msg err';
  }
};

$('btn-exit-sync').onclick = async () => {
  try {
    API = $('device-url').value.trim().replace(/\/$/, '') || API;
    await setEditorSyncMode(false);
    $('connect-msg').textContent = 'Remote rebooting…';
    $('connect-msg').className = 'msg ok';
  } catch (e) {
    $('connect-msg').textContent = e.message;
    $('connect-msg').className = 'msg err';
  }
};

function initAfterLoad() {
  syncOrphanSceneFiles();
  if (normalizeScenePagePaths()) {
    $('deploy-msg').textContent = 'Fixed scene tab paths (Pages/ prefix). Save to remote when ready.';
    $('deploy-msg').className = 'msg ok';
  }
  if (!files.has(HA_SETTINGS_PATH)) setFile(HA_SETTINGS_PATH, { Url: '', Token: '' }, false);
  loadHaSettingsForm();
  if (!selectedScenePath) {
    const reg = sceneRegistry();
    if (reg.Scenes?.[0]?.FileName) selectedScenePath = reg.Scenes[0].FileName;
    else if (listPaths('Scenes/')[0]) selectedScenePath = listPaths('Scenes/')[0];
  }
  populateCmdFileSelect();
  refreshAll();
}

function refreshAll() {
  refreshScenesTab();
  refreshRemoteTab();
  populateRawSelect();
}

/* ── Scenes (Scenes.json + scene files) ── */
function refreshScenesTab() {
  const ul = $('scene-list');
  ul.innerHTML = '';
  const reg = sceneRegistry();
  (reg.Scenes || []).forEach((s, idx) => {
    const li = document.createElement('li');
    const entry = s.SceneName || s.FileName;
    const bind = s.BindToKey ? ` · ${s.BindToKey}` : '';
    li.textContent = entry + bind;
    li.title = s.FileName;
    li.className = s.FileName === selectedScenePath ? 'active' : '';
    li.onclick = () => {
      selectedScenePath = s.FileName;
      activeTabIdx = 0;
      refreshScenesTab();
      refreshRemoteTab();
    };
    ul.appendChild(li);
  });

  const hasSel = !!selectedScenePath && files.has(selectedScenePath);
  $('scene-empty').classList.toggle('hidden', hasSel);
  $('scene-editor').classList.toggle('hidden', !hasSel);
  if (!hasSel) {
    $('scene-title').textContent = 'Select a scene';
    return;
  }

  const entry = sceneRegistryEntry();
  const scene = parseJson(selectedScenePath);
  const warn = $('scene-parse-warn');
  if (warn) {
    if (files.has(selectedScenePath) && !scene) {
      warn.textContent = 'Could not parse this scene file. Open it in Advanced → JSON to fix syntax errors.';
      warn.classList.remove('hidden');
    } else {
      warn.classList.add('hidden');
    }
  }
  const pickerName = entry?.SceneName || scene?.ScreenName || 'Scene';
  $('scene-title').textContent = pickerName;
  $('scene-picker-name').value = entry?.SceneName || '';
  $('scene-screen-name').value = scene?.ScreenName || '';
  if ($('scene-bind-key')) $('scene-bind-key').value = entry?.BindToKey || '';
  if ($('scene-press-type')) $('scene-press-type').value = entry?.PressType || 'Press';
  renderDeviceTabList(scene);
  renderCommandSequences(scene);
}

function addDeviceTabFromUi(templateSelectId) {
  if (!selectedScenePath) return;
  const picked = promptNewDeviceTab();
  if (!picked) return;
  const tpl = $(templateSelectId)?.value || 'blank';
  addDeviceToScene(picked.pageName, tpl, picked.shortName);
}

$('scene-picker-name').oninput = () => {
  const reg = sceneRegistry();
  const hit = reg.Scenes?.find((s) => s.FileName === selectedScenePath);
  if (!hit) return;
  hit.SceneName = $('scene-picker-name').value.trim();
  setSceneRegistry(reg);
  refreshScenesTab();
};

$('scene-screen-name').oninput = () => {
  const scene = parseJson(selectedScenePath) || {};
  scene.ScreenName = $('scene-screen-name').value.trim();
  setFile(selectedScenePath, scene);
  refreshScenesTab();
};

function saveSceneRegistryFields() {
  const reg = sceneRegistry();
  const hit = reg.Scenes?.find((s) => s.FileName === selectedScenePath);
  if (!hit) return;
  const bind = $('scene-bind-key')?.value || '';
  if (bind) {
    hit.BindToKey = bind;
    hit.PressType = $('scene-press-type')?.value || 'Press';
  } else {
    delete hit.BindToKey;
    delete hit.PressType;
  }
  setSceneRegistry(reg);
  refreshScenesTab();
}

$('scene-bind-key')?.addEventListener('change', saveSceneRegistryFields);
$('scene-press-type')?.addEventListener('change', saveSceneRegistryFields);

$('btn-new-scene').onclick = () => {
  const name = prompt('Scene name (shown in remote scene picker):', 'Watch TV');
  if (!name?.trim()) return;
  addNewScene(name.trim());
};

$('btn-delete-scene').onclick = deleteSelectedScene;

$('btn-add-device').onclick = () => addDeviceTabFromUi('device-template');

$('btn-remote-add-tab').onclick = () => addDeviceTabFromUi('remote-device-template');

$('btn-configure-remote').onclick = () => showTab('remote');

function renderCommandSequences(scene) {
  const renderSeq = (containerId, key) => {
    const box = $(containerId);
    if (!box) return;
    box.innerHTML = '';
    (scene?.[key] || []).forEach((item, idx) => {
      const row = document.createElement('div');
      row.className = 'seq-row';
      const devSel = document.createElement('select');
      (scene.Pages || []).forEach((pg) => {
        const cf = commandFileForPage(resolvePagePath(pg.FileName));
        const o = document.createElement('option');
        o.value = cf;
        o.textContent = pg.PageName || pg.ShortName || cf;
        if (item.CommandFile === cf) o.selected = true;
        devSel.appendChild(o);
      });
      const cmd = document.createElement('input');
      cmd.placeholder = 'Command (e.g. PWR_ON)';
      cmd.value = item.Command || '';
      const del = document.createElement('button');
      del.type = 'button';
      del.textContent = '×';
      del.onclick = () => {
        const sc = parseJson(selectedScenePath);
        sc[key].splice(idx, 1);
        setFile(selectedScenePath, sc);
        renderCommandSequences(sc);
      };
      const save = () => {
        const sc = parseJson(selectedScenePath);
        sc[key][idx] = { CommandFile: devSel.value, Command: cmd.value };
        setFile(selectedScenePath, sc);
      };
      devSel.onchange = save;
      cmd.oninput = save;
      row.append(devSel, cmd, del);
      box.appendChild(row);
    });
  };
  renderSeq('scene-start-seq', 'StartCommandSequence');
  renderSeq('scene-exit-seq', 'ExitCommandSequence');
}

function addSeqEntry(key) {
  const scene = parseJson(selectedScenePath) || { Pages: [] };
  scene[key] = scene[key] || [];
  const first = scene.Pages?.[0];
  const cf = first ? commandFileForPage(resolvePagePath(first.FileName)) : listPaths('Commands/')[0] || '';
  scene[key].push({ CommandFile: cf, Command: '' });
  setFile(selectedScenePath, scene);
  renderCommandSequences(scene);
}

$('btn-add-start-cmd').onclick = () => addSeqEntry('StartCommandSequence');
$('btn-add-exit-cmd').onclick = () => addSeqEntry('ExitCommandSequence');

/* ── Remote tab: touch screen + physical face ── */
function refreshRemoteTab() {
  const sel = $('remote-scene-select');
  sel.innerHTML = '';
  const reg = sceneRegistry();
  (reg.Scenes || []).forEach((a) => {
    const o = document.createElement('option');
    o.value = a.FileName;
    o.textContent = a.SceneName || a.FileName;
    sel.appendChild(o);
  });
  sel.value = selectedScenePath || '';
  sel.onchange = () => {
    selectedScenePath = sel.value;
    activeTabIdx = 0;
    refreshRemoteTab();
    refreshScenesTab();
  };

  const scene = parseJson(selectedScenePath);
  renderDeviceTabEditor($('remote-device-tabs'), scene, 'remote');

  const ctx = sceneContext();
  if (ctx.pagePath) selectedPagePath = ctx.pagePath;
  if (selectedPagePath !== lastPreviewPagePath) {
    canvasScrollY = 0;
    lastPreviewPagePath = selectedPagePath;
  }
  const entry = scene?.Pages?.[activeTabIdx];
  const regEntry = sceneRegistryEntry();
  const sceneLabel = regEntry?.SceneName || scene?.ScreenName || 'Scene';
  $('remote-device-label').textContent = entry?.PageName || entry?.ShortName || 'Device';
  const hint = $('editing-hint');
  if (hint) hint.textContent = `Scene: ${sceneLabel} · “${entry?.ShortName || entry?.PageName || '?'}" — edit screen & keys on the remote.`;
  const pg = currentPage();
  const linkedHint = $('linked-files-hint');
  if (linkedHint) linkedHint.textContent = pg.CommandFile ? `Linked: ${selectedPagePath} → ${pg.CommandFile}` : '';
  renderWidgetList(pg);
  drawCanvas();
  renderRemoteKeymap();
  updateSelectionPanel();
  refreshHaPreviewStates().catch(() => {});
}

function tabLabels() {
  const scene = parseJson(selectedScenePath);
  return (scene?.Pages || []).map((p, i) => p.ShortName || p.PageName || `Tab ${i + 1}`);
}

function renderRemoteKeymap() {
  const powerRow = $('remote-power-row');
  const face = $('remote-face');
  if (!powerRow || !face) return;
  powerRow.innerHTML = '';
  face.innerHTML = '';
  const page = currentPage();

  const makeBtn = (keyId, label, shape, extra = '') => {
    const mapped = getKeyMapping(keyId);
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'remote-key' + (shape ? ' ' + shape : '') + (mapped ? ' mapped' : '') +
      (selection.kind === 'key' && selection.keyName === keyId ? ' selected' : '') + extra;
    btn.innerHTML = `<span class="rk-lbl">${label}</span><span class="rk-map">${describeCommand(mapped, selectedPagePath) || '—'}</span>`;
    btn.onclick = () => selectKey(keyId, label);
    return btn;
  };

  powerRow.appendChild(makeBtn('Power', 'Power', 'shape-power'));

  const media = document.createElement('div');
  media.className = 'remote-media';
  [['Stop', 'Stop'], ['Rewind', 'Rewind'], ['Play', 'Play'], ['FastForward', 'Forward']].forEach(([k, l]) => {
    media.appendChild(makeBtn(k, l, 'shape-round'));
  });
  face.appendChild(media);

  const nav = document.createElement('div');
  nav.className = 'remote-nav';
  nav.appendChild(makeBtn('Menu', 'Menu', 'shape-corner corner-tl'));
  nav.appendChild(makeBtn('Info', 'Info', 'shape-corner corner-tr'));
  nav.appendChild(makeBtn('Back', 'Back', 'shape-corner corner-bl'));
  nav.appendChild(makeBtn('Source', 'Source', 'shape-corner corner-br'));
  const dpad = document.createElement('div');
  dpad.className = 'remote-dpad';
  dpad.appendChild(makeBtn('Up', 'Up', 'shape-dpad dpad-up'));
  dpad.appendChild(makeBtn('Left', 'Left', 'shape-dpad dpad-left'));
  dpad.appendChild(makeBtn('Center', 'OK', 'shape-dpad-ok dpad-ok'));
  dpad.appendChild(makeBtn('Right', 'Right', 'shape-dpad dpad-right'));
  dpad.appendChild(makeBtn('Down', 'Down', 'shape-dpad dpad-down'));
  nav.appendChild(dpad);
  face.appendChild(nav);

  const rockers = document.createElement('div');
  rockers.className = 'remote-rockers';
  const vol = document.createElement('div');
  vol.className = 'remote-rocker';
  vol.appendChild(makeBtn('VolUp', 'Vol+', 'shape-rocker-tall'));
  vol.appendChild(makeBtn('VolDown', 'Vol-', 'shape-rocker-tall'));
  const mid = document.createElement('div');
  mid.className = 'remote-rocker mid';
  mid.appendChild(makeBtn('Mute', 'Mute', 'shape-round'));
  mid.appendChild(makeBtn('Record', 'Record', 'shape-round'));
  const ch = document.createElement('div');
  ch.className = 'remote-rocker';
  ch.appendChild(makeBtn('ChannelUp', 'CH+', 'shape-rocker-tall'));
  ch.appendChild(makeBtn('ChannelDown', 'CH-', 'shape-rocker-tall'));
  rockers.append(vol, mid, ch);
  face.appendChild(rockers);

  const colors = document.createElement('div');
  colors.className = 'remote-colors';
  [['Aux1', 'Red', 'color-red'], ['Aux2', 'Green', 'color-green'], ['Aux3', 'Yellow', 'color-yellow'], ['Aux4', 'Blue', 'color-blue']].forEach(([k, l, c]) => {
    colors.appendChild(makeBtn(k, l, 'shape-round', ' ' + c));
  });
  face.appendChild(colors);
}

function clearSelection() {
  selection = { kind: null, widgetIdx: null, keyName: null };
  selectedWidgetIdx = -1;
  selectedKeyName = '';
  updateSelectionPanel();
  renderRemoteKeymap();
  drawCanvas();
}

function selectWidget(idx) {
  selection = { kind: 'widget', widgetIdx: idx, keyName: null };
  selectedWidgetIdx = idx;
  selectedKeyName = '';
  updateSelectionPanel();
  renderWidgetList(currentPage());
  renderRemoteKeymap();
  drawCanvas();
}

function selectKey(keyId, label) {
  selection = { kind: 'key', widgetIdx: null, keyName: keyId };
  selectedKeyName = keyId;
  selectedWidgetIdx = -1;
  updateSelectionPanel(label || KEY_LABELS[keyId] || keyId);
  renderWidgetList(currentPage());
  renderRemoteKeymap();
  drawCanvas();
}

function activePageName() {
  const scene = parseJson(selectedScenePath);
  const entry = scene?.Pages?.[activeTabIdx];
  return entry?.PageName || entry?.ShortName || 'Page';
}

function populateWidgetTypeSelects() {
  const opts = Object.entries(WIDGET_TYPES)
    .map(([k, v]) => `<option value="${k}">${v.label}</option>`).join('');
  if ($('w-type')) $('w-type').innerHTML = opts;
  if ($('add-widget-type')) $('add-widget-type').innerHTML = opts;
}

function populateImageFileList() {
  const dl = $('image-file-list');
  if (!dl) return;
  dl.innerHTML = '';
  listPaths('Images/').forEach((p) => {
    const o = document.createElement('option');
    o.value = p;
    dl.appendChild(o);
  });
}

function ensureStubCommands(cmdFile, names) {
  if (!cmdFile || !names?.length) return;
  const doc = parseJson(cmdFile) || { Manufacturer: 'Custom', DeviceClass: 'Generic', Commands: [] };
  doc.Commands = doc.Commands || [];
  let changed = false;
  names.forEach((n) => {
    if (!doc.Commands.some((c) => c.Command === n)) {
      doc.Commands.push({ Command: n, Mode: 'IR', Protocol: 'NEC', Data: ['0x0'] });
      changed = true;
    }
  });
  if (changed) setFile(cmdFile, doc);
}

function widgetSummary(w) {
  if (w.Type === 'Image') return w.FileName || 'Image';
  if (w.Type === 'ColorButtons') return 'RGYB keys';
  if (w.Type === 'NumberPad') return '0–9 pad';
  if (w.Type === 'Title') return activePageName();
  if (w.Type === 'HaToggle' || w.Type === 'HaLabel') {
    const name = w.Text || w.Type;
    return w.EntityId ? `${name} → ${w.EntityId}` : name;
  }
  const cmd = Array.isArray(w.Command) ? w.Command[0] : w.Command;
  const text = w.Text || w.Type;
  return cmd ? `${text} → ${cmd}` : text;
}

function deleteWidgetAt(idx) {
  const page = currentPage();
  const w = page.Widgets?.[idx];
  if (!w) return;
  const label = widgetSummary(w);
  if (!confirm(`Delete widget “${label}”?`)) return;
  page.Widgets.splice(idx, 1);
  page.Widgets.forEach((wg, i) => {
    if (wg.AlignTo != null && wg.AlignTo > idx) wg.AlignTo -= 1;
  });
  savePage(page);
  clearSelection();
  refreshRemoteTab();
}

function addWidgetOfType(type) {
  const spec = WIDGET_TYPES[type];
  if (!spec) return;
  const page = currentPage();
  page.Widgets = page.Widgets || [];
  const widget = spec.create(page.Widgets);
  if (spec.stubCommands) {
    const cf = ensurePageCommandFile();
    ensureStubCommands(cf, spec.stubCommands);
  }
  page.Widgets.push(widget);
  savePage(page);
  selectWidget(page.Widgets.length - 1);
  if (type === 'HaToggle' || type === 'HaLabel') {
    syncWidgetEditor();
    populateHaEntityPicker({ autoApplyLabel: false }).catch(() => {});
  }
}

function syncWidgetEditor() {
  const w = selection.kind === 'widget' ? currentPage().Widgets?.[selection.widgetIdx] : null;
  const type = w?.Type || 'Button';
  const spec = WIDGET_TYPES[type];

  $('panel-key-editor')?.classList.toggle('hidden', selection.kind !== 'key');
  $('panel-widget-editor')?.classList.toggle('hidden', selection.kind !== 'widget');

  if (selection.kind !== 'widget' || !w) return;

  if ($('w-type-hint')) $('w-type-hint').textContent = spec?.hint || '';

  const isButton = type === 'Button';
  const isLabel = type === 'Label';
  const isTitle = type === 'Title';
  const isImage = type === 'Image';
  const isColor = type === 'ColorButtons';
  const isPad = type === 'NumberPad';
  const isHaToggle = type === 'HaToggle';
  const isHaLabel = type === 'HaLabel';
  const isHa = isHaToggle || isHaLabel;

  $('w-fields-text')?.classList.toggle('hidden', isTitle || isImage || isColor || isPad || isHaLabel);
  $('w-fields-command')?.classList.toggle('hidden', !(isButton || isLabel));
  $('w-fields-image')?.classList.toggle('hidden', !isImage);
  $('w-fields-color')?.classList.toggle('hidden', !isColor);
  $('w-fields-numpad')?.classList.toggle('hidden', !isPad);
  $('w-fields-ha')?.classList.toggle('hidden', !isHa);
  $('w-fields-layout')?.classList.toggle('hidden', isColor || isPad);

  if ($('ha-service-row')) {
    $('ha-service-row').classList.toggle('hidden', !isHaToggle);
  }

  if ($('w-text-label')) {
    $('w-text-label').firstChild.textContent = isLabel || isHaLabel ? 'Text ' : 'Label ';
  }

  if (isHa) {
    $('action-touch-label').value = w.Text || '';
    if (w.EntityId) {
      haActiveDomain = w.EntityId.split('.')[0] || haActiveDomain;
    }
    renderHaDomainTabs();
    if ($('ha-service') && w.Service) $('ha-service').value = w.Service;
    populateHaEntityPicker({ selected: w.EntityId || '', autoApplyLabel: false });
  }

  if (isTitle && $('w-fields-layout')) {
    $('w-fields-layout').classList.remove('hidden');
    $('w-height').parentElement.classList.remove('hidden');
  }

  if (isButton || isLabel) {
    const hasCmd = !!(w.Command && (typeof w.Command === 'string' ? w.Command : w.Command[0]));
    $('widget-action-type').value = hasCmd ? 'ir_existing' : (w.Text && !hasCmd ? 'none' : 'ir');
    if (isLabel && hasCmd) $('widget-action-type').value = 'ir_existing';
  }

  $('widget-panel-ir')?.classList.toggle('hidden', $('widget-action-type')?.value !== 'ir');
  $('widget-panel-existing')?.classList.toggle('hidden', $('widget-action-type')?.value !== 'ir_existing');

  if (isImage) {
    $('w-image-file').value = w.FileName || '';
    $('w-image-w').value = w.SizeXYinPixels?.[0] ?? 120;
    $('w-image-h').value = w.SizeXYinPixels?.[1] ?? 120;
    populateImageFileList();
  }

  if (isColor && Array.isArray(w.Command)) {
    $('w-cmd-red').value = w.Command[0] || 'RED';
    $('w-cmd-green').value = w.Command[1] || 'GREEN';
    $('w-cmd-yellow').value = w.Command[2] || 'YELLOW';
    $('w-cmd-blue').value = w.Command[3] || 'BLUE';
  }

  const showHeight = isButton || isLabel || isTitle || isHa;
  if ($('w-height')?.parentElement) {
    $('w-height').parentElement.classList.toggle('hidden', !showHeight);
  }
  if (showHeight) $('w-height').value = w.HeightPct ?? 10;

  if ($('w-sizex')) $('w-sizex').value = w.SizeXY?.[0] ?? '';
  if ($('w-sizey')) $('w-sizey').value = w.SizeXY?.[1] ?? '';
  if ($('w-posx')) $('w-posx').value = w.PosX ?? '';
  if ($('w-posy')) $('w-posy').value = w.PosY ?? '';
}

function updateSelectionPanel(friendlyLabel) {
  const empty = $('selection-empty');
  const editor = $('selection-editor');
  if (!selection.kind) {
    empty?.classList.remove('hidden');
    editor?.classList.add('hidden');
    return;
  }
  empty?.classList.add('hidden');
  editor?.classList.remove('hidden');

  if (selection.kind === 'widget') {
    const w = currentPage().Widgets?.[selection.widgetIdx];
    const typeLabel = WIDGET_TYPES[w?.Type]?.label || w?.Type || 'Widget';
    $('selection-title').textContent = typeLabel;
    if (w?.Type === 'NumberPad' || w?.Type === 'ColorButtons') {
      $('selection-sub').textContent = 'Fixed layout · drag to reorder via widget list';
    } else if (DRAGGABLE_WIDGET_TYPES.has(w?.Type)) {
      $('selection-sub').textContent = `${widgetSummary(w)} · drag on preview to move`;
    } else {
      $('selection-sub').textContent = widgetSummary(w);
    }
    if ($('w-type')) $('w-type').value = w?.Type || 'Button';
    $('action-touch-label').value = w?.Text || '';
    populateWidgetCmdPick(w);
    syncWidgetEditor();
  } else {
    $('selection-title').textContent = 'Physical key';
    $('selection-sub').textContent = friendlyLabel || KEY_LABELS[selection.keyName] || selection.keyName;
    const mapped = getKeyMapping(selection.keyName, $('key-press-type')?.value || 'Press');
    $('action-type').value = mapped ? 'ir_existing' : 'ir';
    populateActionCmdPick(mapped);
    $('action-cmd-name').value = DEFAULT_CMD_FOR_KEY[selection.keyName] || selection.keyName.toUpperCase();
    syncWidgetEditor();
  }
  syncActionPanels();
}

function populateWidgetCmdPick(w) {
  const sel = $('widget-cmd-pick');
  if (!sel) return;
  sel.innerHTML = '<option value="">— pick —</option>';
  const cf = ensurePageCommandFile();
  const selected = typeof w?.Command === 'string' ? w.Command : '';
  commandNamesFromFile(cf).forEach((n) => {
    const o = document.createElement('option');
    o.value = n;
    o.textContent = describeCommand(n, selectedPagePath);
    if (n === selected) o.selected = true;
    sel.appendChild(o);
  });
}

function populateActionCmdPick(selected) {
  const sel = $('action-cmd-pick');
  sel.innerHTML = '<option value="">— pick —</option>';
  const cf = ensurePageCommandFile();
  commandNamesFromFile(cf).forEach((n) => {
    const o = document.createElement('option');
    o.value = n;
    o.textContent = describeCommand(n, selectedPagePath);
    if (n === selected) o.selected = true;
    sel.appendChild(o);
  });
}

function syncActionPanels() {
  const t = $('action-type')?.value;
  $('panel-ir')?.classList.toggle('hidden', t !== 'ir');
  $('panel-ir-existing')?.classList.toggle('hidden', t !== 'ir_existing');
  $('panel-key-advanced')?.classList.toggle('hidden', selection.kind !== 'key');
}

$('action-type')?.addEventListener('change', syncActionPanels);
$('widget-action-type')?.addEventListener('change', () => {
  $('widget-panel-ir')?.classList.toggle('hidden', $('widget-action-type').value !== 'ir');
  $('widget-panel-existing')?.classList.toggle('hidden', $('widget-action-type').value !== 'ir_existing');
});

$('w-type')?.addEventListener('change', () => {
  if (selection.kind !== 'widget') return;
  const page = currentPage();
  const w = page.Widgets?.[selection.widgetIdx];
  if (!w) return;
  const newType = $('w-type').value;
  const fresh = WIDGET_TYPES[newType]?.create(page.Widgets.filter((_, i) => i !== selection.widgetIdx)) || { Type: newType };
  fresh.AlignTo = w.AlignTo ?? selection.widgetIdx;
  page.Widgets[selection.widgetIdx] = fresh;
  if (WIDGET_TYPES[newType]?.stubCommands) {
    ensureStubCommands(ensurePageCommandFile(), WIDGET_TYPES[newType].stubCommands);
  }
  savePage(page);
  updateSelectionPanel();
  drawCanvas();
  renderWidgetList(page);
});

function applyWidgetEdits() {
  const page = currentPage();
  const w = page.Widgets?.[selection.widgetIdx];
  if (!w) return;

  const type = w.Type || 'Button';
  if (type === 'HaToggle' || type === 'HaLabel') {
    w.Text = $('action-touch-label').value.trim();
    applyHaEntityPickToWidget();
    if (type === 'HaToggle') {
      w.Service = $('ha-service')?.value || defaultHaService(w.Domain, type);
    }
  } else if (type === 'Button' || type === 'Label') {
    w.Text = $('action-touch-label').value.trim();
    const wa = $('widget-action-type').value;
    if (wa === 'none') delete w.Command;
    else if (wa === 'ir_existing') w.Command = $('widget-cmd-pick').value;
  }

  if (type === 'Image') {
    w.FileName = $('w-image-file').value.trim() || 'Images/OMOTE_Logo.png';
    const iw = parseInt($('w-image-w').value, 10);
    const ih = parseInt($('w-image-h').value, 10);
    w.SizeXYinPixels = [iw || 120, ih || 120];
  }

  if (type === 'ColorButtons') {
    w.Command = [
      $('w-cmd-red').value.trim() || 'RED',
      $('w-cmd-green').value.trim() || 'GREEN',
      $('w-cmd-yellow').value.trim() || 'YELLOW',
      $('w-cmd-blue').value.trim() || 'BLUE'
    ];
    ensureStubCommands(ensurePageCommandFile(), w.Command);
  }

  if (type === 'Button' || type === 'Label' || type === 'Title' || type === 'HaToggle' || type === 'HaLabel') {
    const h = parseInt($('w-height').value, 10);
    if (h) w.HeightPct = h;
  }

  if (type === 'Button') {
    const sx = parseInt($('w-sizex')?.value, 10);
    const sy = parseInt($('w-sizey')?.value, 10);
    if (sx && sy) w.SizeXY = [sx, sy];
    else delete w.SizeXY;
  }

  const px = parseInt($('w-posx')?.value, 10);
  const py = parseInt($('w-posy')?.value, 10);
  if (!Number.isNaN(px) && $('w-posx')?.value !== '') {
    w.PosX = Math.round(clampWidgetX(SCR_W * px / 100, widgetLayoutWidth(w)) / SCR_W * 100);
    delete w.AlignTo;
  }
  if (!Number.isNaN(py) && $('w-posy')?.value !== '') {
    w.PosY = Math.max(0, py);
    delete w.AlignTo;
  }

  savePage(page);
  refreshRemoteTab();
}

$('btn-widget-apply').onclick = applyWidgetEdits;

$('btn-key-apply').onclick = () => {
  const t = $('action-type').value;
  const page = currentPage();
  page.ButtonMaps = page.ButtonMaps || {};
  const pt = $('key-press-type').value;
  const cmd = t === 'ir_existing' ? $('action-cmd-pick').value : $('action-cmd-name').value.trim();
  if (!cmd) return;
  page.ButtonMaps[selection.keyName] = page.ButtonMaps[selection.keyName] || {};
  page.ButtonMaps[selection.keyName][pt] = cmd;
  savePage(page);
  refreshRemoteTab();
};

$('btn-key-clear').onclick = () => {
  const page = currentPage();
  const pt = $('key-press-type').value;
  if (page.ButtonMaps?.[selection.keyName]?.[pt]) {
    delete page.ButtonMaps[selection.keyName][pt];
    if (!Object.keys(page.ButtonMaps[selection.keyName]).length) delete page.ButtonMaps[selection.keyName];
  }
  savePage(page);
  refreshRemoteTab();
};

$('btn-delete-widget').onclick = () => {
  if (selection.kind !== 'widget') return;
  deleteWidgetAt(selection.widgetIdx);
};

$('btn-widget-learn').onclick = async () => {
  const msg = $('widget-learn-msg');
  try {
    const cap = await learnIr(msg);
    const cf = ensurePageCommandFile();
    const page = currentPage();
    const w = page.Widgets[selection.widgetIdx];
    if (!w) return;
    const cmdName = (w.Text || 'BTN').toUpperCase().replace(/\s+/g, '_');
    w.Command = cmdName;
    upsertCommand(cf, cmdName, cap.protocol, cap.code);
    populateCmdFileSelect();
    populateWidgetCmdPick(w);
    $('widget-action-type').value = 'ir_existing';
    syncWidgetEditor();
    savePage(page);
    refreshRemoteTab();
  } catch (e) {
    msg.textContent = e.message;
    msg.className = 'msg err small';
  }
};

$('btn-action-learn').onclick = async () => {
  const msg = $('action-learn-msg');
  try {
    const cap = await learnIr(msg);
    const cf = ensurePageCommandFile();
    let cmdName;
    if (selection.kind === 'key') {
      cmdName = $('action-cmd-name').value.trim() || DEFAULT_CMD_FOR_KEY[selection.keyName] || selection.keyName.toUpperCase();
      const page = currentPage();
      page.ButtonMaps = page.ButtonMaps || {};
      const pt = $('key-press-type').value;
      page.ButtonMaps[selection.keyName] = page.ButtonMaps[selection.keyName] || {};
      page.ButtonMaps[selection.keyName][pt] = cmdName;
      savePage(page);
    } else return;
    upsertCommand(cf, cmdName, cap.protocol, cap.code);
    populateCmdFileSelect();
    refreshRemoteTab();
  } catch (e) {
    msg.textContent = e.message;
    msg.className = 'msg err';
  }
};

function renderWidgetList(page) {
  const ul = $('widget-list');
  if (!ul) return;
  ul.innerHTML = '';
  (page?.Widgets || []).forEach((w, i) => {
    const li = document.createElement('li');
    li.className = 'widget-list-item';
    if (selection.kind === 'widget' && selection.widgetIdx === i) li.classList.add('selected');

    const badge = document.createElement('span');
    badge.className = 'widget-type-badge';
    badge.textContent = (w.Type || '?').replace('ColorButtons', 'Colors').replace('NumberPad', 'Numpad');

    const text = document.createElement('span');
    text.className = 'widget-list-label';
    text.textContent = widgetSummary(w);

    const del = document.createElement('button');
    del.type = 'button';
    del.className = 'icon-btn danger';
    del.textContent = '×';
    del.title = 'Delete widget';
    del.onclick = (e) => {
      e.stopPropagation();
      deleteWidgetAt(i);
    };

    li.append(badge, text, del);
    li.onclick = () => selectWidget(i);
    ul.appendChild(li);
  });
}

$('btn-add-widget').onclick = () => {
  const type = $('add-widget-type')?.value || 'Button';
  addWidgetOfType(type);
};

populateWidgetTypeSelects();

/* ── Canvas (layout mirrors JsonPage + LVGL++ widget sizes) ── */
function widgetLayoutHeight(w) {
  if (w.Type === 'ColorButtons') return FW_LAYOUT.colorButtons.height;
  if (w.Type === 'NumberPad') return FW_LAYOUT.numberPad.height;
  if (w.Type === 'Image' && w.SizeXYinPixels?.[1]) return w.SizeXYinPixels[1];
  const hp = w.HeightPct || 10;
  return Math.max(16, Math.round((CONTENT_H - FW_LAYOUT.gap * 2) * hp / 100));
}

function widgetLayoutWidth(w) {
  if (w.Type === 'ColorButtons') {
    const c = FW_LAYOUT.colorButtons;
    return c.marginX * 2 + c.btnW * 4 + c.spacingX * 3;
  }
  if (w.Type === 'NumberPad') {
    const n = FW_LAYOUT.numberPad;
    return n.pad * 2 + n.btnW * n.cols + n.spacingX * (n.cols - 1);
  }
  if (w.SizeXY?.[0]) return Math.round(SCR_W * w.SizeXY[0] / 100);
  if (w.Type === 'Image' && w.SizeXYinPixels?.[0]) return w.SizeXYinPixels[0];
  return SCR_W - FW_LAYOUT.padX * 2;
}

function clampWidgetX(x, width) {
  return Math.max(0, Math.min(SCR_W - width, x));
}

function computeWidgetLayout(widgets) {
  const flowBottoms = [];
  const items = (widgets || []).map((w, i) => {
    const h = widgetLayoutHeight(w);
    const width = widgetLayoutWidth(w);
    const alignTo = w.AlignTo != null ? w.AlignTo : 0;
    let flowY;
    if (alignTo === 0) {
      flowY = STATUS_H + FW_LAYOUT.gap;
    } else {
      const refIdx = Math.min(alignTo - 1, i - 1);
      flowY = (flowBottoms[refIdx] ?? STATUS_H + FW_LAYOUT.gap) + FW_LAYOUT.gap;
    }
    flowBottoms[i] = flowY + h;
    const flowX = clampWidgetX(Math.round((SCR_W - width) / 2), width);
    const positioned = w.PosX != null && w.PosY != null;
    return { i, w, h, width, flowX, flowY, positioned };
  });

  const virtualH = Math.max(
    CONTENT_H,
    (flowBottoms.length ? flowBottoms[flowBottoms.length - 1] : STATUS_H + FW_LAYOUT.gap) - STATUS_H + FW_LAYOUT.gap
  );

  return items.map((item) => {
    let x;
    let y;
    if (item.positioned) {
      x = clampWidgetX(Math.round(SCR_W * item.w.PosX / 100), item.width);
      y = STATUS_H + Math.round(virtualH * item.w.PosY / 100);
    } else {
      x = item.flowX;
      y = item.flowY;
    }
    return {
      i: item.i,
      x,
      y,
      rawY: y,
      w: item.width,
      h: item.h,
      widget: item.w
    };
  });
}

function layoutFlowRects(widgets, applyScroll = false) {
  const scroll = applyScroll ? canvasScrollY : 0;
  return computeWidgetLayout(widgets).map((r) => ({
    ...r,
    y: r.y - scroll
  }));
}

function layoutVirtualHeight(widgets) {
  const items = computeWidgetLayout(widgets);
  if (!items.length) return CONTENT_H;
  const last = items[items.length - 1];
  return Math.max(CONTENT_H, last.y + last.h - STATUS_H + FW_LAYOUT.gap);
}

function maxCanvasScroll(widgets) {
  const items = computeWidgetLayout(widgets);
  if (!items.length) return 0;
  const last = items[items.length - 1];
  const visibleBottom = SCR_H - TAB_BAR_H;
  return Math.max(0, last.y + last.h - visibleBottom + FW_LAYOUT.gap);
}

function updateCanvasScrollHint(widgets) {
  const hint = $('canvas-scroll-hint');
  if (!hint) return;
  hint.classList.toggle('hidden', maxCanvasScroll(widgets) <= 0);
}

function drawMiniButton(ctx, x, y, w, h, label) {
  ctx.fillStyle = '#555';
  ctx.fillRect(x, y, w, h);
  ctx.strokeStyle = '#888';
  ctx.strokeRect(x, y, w, h);
  ctx.fillStyle = '#eee';
  ctx.font = '11px sans-serif';
  ctx.textAlign = 'center';
  ctx.fillText(label, x + w / 2, y + h / 2 + 4);
  ctx.textAlign = 'left';
}

function drawColorButtonsWidget(ctx, r) {
  const c = FW_LAYOUT.colorButtons;
  const colors = ['#c0392b', '#27ae60', '#f1c40f', '#2980b9'];
  ctx.strokeStyle = '#444';
  ctx.strokeRect(r.x, r.y, r.w, r.h);
  colors.forEach((col, ci) => {
    const bx = r.x + c.marginX + ci * (c.btnW + c.spacingX);
    ctx.fillStyle = col;
    ctx.fillRect(bx, r.y, c.btnW, r.h);
    ctx.strokeStyle = '#333';
    ctx.strokeRect(bx, r.y, c.btnW, r.h);
  });
}

function drawNumberPadWidget(ctx, r) {
  const n = FW_LAYOUT.numberPad;
  ctx.strokeStyle = '#444';
  ctx.strokeRect(r.x, r.y, r.w, r.h);
  for (let num = 1; num <= 9; num++) {
    const idx = num - 1;
    const col = idx % n.cols;
    const row = Math.floor(idx / n.cols);
    const bx = r.x + n.pad + col * (n.btnW + n.spacingX);
    const by = r.y + n.pad + row * (n.btnH + n.spacingY);
    drawMiniButton(ctx, bx, by, n.btnW, n.btnH, String(num));
  }
  const bx0 = r.x + n.pad + 1 * (n.btnW + n.spacingX);
  const by0 = r.y + n.pad + 3 * (n.btnH + n.spacingY);
  drawMiniButton(ctx, bx0, by0, n.btnW, n.btnH, '0');
}

function drawCanvas() {
  const c = $('preview');
  if (!c) return;
  const ctx = c.getContext('2d');
  const widgets = currentPage().Widgets || [];

  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, SCR_W, SCR_H);
  ctx.fillStyle = '#222';
  ctx.fillRect(0, 0, SCR_W, STATUS_H);
  ctx.fillStyle = '#aaa';
  ctx.font = '11px sans-serif';
  const scene = parseJson(selectedScenePath);
  ctx.fillText(scene?.ScreenName ? `Scene:${scene.ScreenName}` : 'OMOTE', 8, 15);

  const contentTop = STATUS_H;
  const contentBottom = SCR_H - TAB_BAR_H;
  const rects = layoutFlowRects(widgets, true);

  ctx.save();
  ctx.beginPath();
  ctx.rect(0, contentTop, SCR_W, contentBottom - contentTop);
  ctx.clip();

  rects.forEach((r) => {
    const w = r.widget;
    const sel = selection.kind === 'widget' && selection.widgetIdx === r.i;
    ctx.lineWidth = sel ? 2 : 1;
    ctx.strokeStyle = sel ? '#58a6ff' : '#555';

    if (w.Type === 'ColorButtons') {
      drawColorButtonsWidget(ctx, r);
    } else if (w.Type === 'NumberPad') {
      drawNumberPadWidget(ctx, r);
    } else if (w.Type === 'Image') {
      ctx.fillStyle = '#1a1a2e';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.strokeStyle = sel ? '#58a6ff' : '#555';
      ctx.strokeRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#888';
      ctx.font = '10px sans-serif';
      ctx.textAlign = 'center';
      const fname = (w.FileName || 'image').split('/').pop();
      ctx.fillText('IMG ' + fname.slice(0, 14), r.x + r.w / 2, r.y + r.h / 2 + 3);
      ctx.textAlign = 'left';
    } else if (w.Type === 'Title') {
      ctx.fillStyle = '#1e3a5f';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#eee';
      ctx.font = 'bold 12px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(activePageName().slice(0, 22), r.x + r.w / 2, r.y + r.h / 2 + 4);
      ctx.textAlign = 'left';
      ctx.strokeStyle = sel ? '#58a6ff' : '#555';
      ctx.strokeRect(r.x, r.y, r.w, r.h);
    } else if (w.Type === 'Label') {
      ctx.fillStyle = '#252530';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#ccc';
      ctx.font = '12px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText((w.Text || 'Label').slice(0, 24), r.x + r.w / 2, r.y + r.h / 2 + 4);
      ctx.textAlign = 'left';
      ctx.strokeStyle = sel ? '#58a6ff' : '#555';
      ctx.strokeRect(r.x, r.y, r.w, r.h);
    } else if (w.Type === 'HaToggle') {
      const st = haStateCache.get(w.EntityId);
      const on = isHaStateOn(st);
      ctx.fillStyle = on ? '#2d6a4f' : '#2a3548';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#eee';
      ctx.font = '12px sans-serif';
      const label = (w.Text || w.EntityId || 'HA').slice(0, 20);
      ctx.fillText(label, r.x + 6, r.y + r.h / 2 + 4);
      ctx.fillStyle = on ? '#7dffb0' : '#666';
      ctx.font = '10px sans-serif';
      ctx.textAlign = 'right';
      ctx.fillText(st != null ? String(st).slice(0, 10) : '…', r.x + r.w - 6, r.y + r.h / 2 + 4);
      ctx.textAlign = 'left';
      ctx.strokeStyle = sel ? '#58a6ff' : '#3d8';
      ctx.strokeRect(r.x, r.y, r.w, r.h);
    } else if (w.Type === 'HaLabel') {
      const st = haStateCache.get(w.EntityId);
      ctx.fillStyle = '#252530';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#9cf';
      ctx.font = '12px sans-serif';
      ctx.textAlign = 'center';
      const line = st != null ? String(st) : (w.Text || '—');
      ctx.fillText(line.slice(0, 24), r.x + r.w / 2, r.y + r.h / 2 + 4);
      ctx.textAlign = 'left';
      ctx.strokeStyle = sel ? '#58a6ff' : '#555';
      ctx.strokeRect(r.x, r.y, r.w, r.h);
    } else {
      ctx.fillStyle = '#2a3548';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#eee';
      ctx.font = '12px sans-serif';
      const label = (w.Text || w.Type || '').slice(0, 22);
      ctx.fillText(label, r.x + 6, r.y + r.h / 2 + 4);
      ctx.strokeRect(r.x, r.y, r.w, r.h);
    }

    if (w.Type === 'ColorButtons' || w.Type === 'NumberPad') {
      ctx.strokeStyle = sel ? '#58a6ff' : '#555';
      ctx.lineWidth = sel ? 2 : 1;
      ctx.strokeRect(r.x, r.y, r.w, r.h);
    }
  });

  ctx.restore();

  const labels = tabLabels();
  const tabY = SCR_H - TAB_BAR_H;
  ctx.fillStyle = '#1a2332';
  ctx.fillRect(0, tabY, SCR_W, TAB_BAR_H);
  if (labels.length) {
    const tw = SCR_W / labels.length;
    labels.forEach((lab, i) => {
      ctx.fillStyle = i === activeTabIdx ? '#388bfd' : '#2d3a4d';
      ctx.fillRect(i * tw + 1, tabY + 1, tw - 2, TAB_BAR_H - 2);
      ctx.fillStyle = i === activeTabIdx ? '#fff' : '#8b949e';
      ctx.font = '10px sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText(String(lab).slice(0, 8), i * tw + tw / 2, tabY + TAB_BAR_H / 2 + 3);
    });
    ctx.textAlign = 'left';
  }

  const maxScroll = maxCanvasScroll(widgets);
  if (maxScroll > 0) {
    const trackH = contentBottom - contentTop - 8;
    const thumbH = Math.max(18, trackH * (CONTENT_H / (CONTENT_H + maxScroll)));
    const thumbY = contentTop + 4 + (trackH - thumbH) * (canvasScrollY / maxScroll);
    ctx.fillStyle = '#ffffff22';
    ctx.fillRect(SCR_W - 5, contentTop + 4, 3, trackH);
    ctx.fillStyle = '#58a6ff88';
    ctx.fillRect(SCR_W - 5, thumbY, 3, thumbH);
  }

  updateCanvasScrollHint(widgets);
}

function canvasCoords(ev) {
  const c = $('preview');
  const rect = c.getBoundingClientRect();
  return {
    x: (ev.clientX - rect.left) * (SCR_W / rect.width),
    y: (ev.clientY - rect.top) * (SCR_H / rect.height)
  };
}

function canvasHitTab(x, y) {
  if (y < SCR_H - TAB_BAR_H) return -1;
  const labels = tabLabels();
  if (!labels.length) return -1;
  return Math.min(labels.length - 1, Math.floor(x / (SCR_W / labels.length)));
}

function findWidgetHit(x, y) {
  const widgets = currentPage().Widgets || [];
  return layoutFlowRects(widgets, true).slice().reverse()
    .find((r) => x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
}

function applyWidgetDragPos(w, x, y, width, height, widgets) {
  const virtualH = layoutVirtualHeight(widgets);
  const px = clampWidgetX(x, width);
  const py = Math.max(0, y - STATUS_H);
  w.PosX = Math.round(px / SCR_W * 100);
  w.PosY = Math.round(py / virtualH * 100);
  delete w.AlignTo;
}

$('preview').onmousedown = (ev) => {
  const { x, y } = canvasCoords(ev);
  const tabHit = canvasHitTab(x, y);
  if (tabHit >= 0) {
    activeTabIdx = tabHit;
    selectedPagePath = resolvePagePath(parseJson(selectedScenePath)?.Pages?.[activeTabIdx]?.FileName);
    clearSelection();
    refreshRemoteTab();
    return;
  }
  if (y > SCR_H - TAB_BAR_H || y < STATUS_H) return;

  const hit = findWidgetHit(x, y);
  if (!hit) { clearSelection(); return; }
  selectWidget(hit.i);

  if (DRAGGABLE_WIDGET_TYPES.has(hit.widget.Type)) {
    drag = {
      idx: hit.i,
      ox: x - hit.x,
      oy: y - hit.y,
      width: hit.w,
      height: hit.h
    };
  }
};

$('preview').onmousemove = (ev) => {
  if (!drag) return;
  const { x, y } = canvasCoords(ev);
  const page = currentPage();
  const w = page.Widgets[drag.idx];
  const nx = clampWidgetX(x - drag.ox, drag.width);
  const ny = y - drag.oy + canvasScrollY;
  applyWidgetDragPos(w, nx, ny, drag.width, drag.height, page.Widgets);
  savePage(page);
  drawCanvas();
};

$('preview').addEventListener('wheel', (ev) => {
  const widgets = currentPage().Widgets || [];
  const maxScroll = maxCanvasScroll(widgets);
  if (maxScroll <= 0) return;
  ev.preventDefault();
  canvasScrollY = Math.max(0, Math.min(maxScroll, canvasScrollY + ev.deltaY));
  drawCanvas();
}, { passive: false });

window.addEventListener('mouseup', () => { drag = null; });

/* ── Advanced: commands + raw ── */
async function learnIr(msgEl) {
  if (msgEl) { msgEl.textContent = 'Learning… point a remote at OMOTE and press a button.'; msgEl.className = 'msg muted small'; }
  await api('/api/ir/learn/start', { method: 'POST' });
  try {
    for (let i = 0; i < 80; i++) {
      await sleep(400);
      const r = await api('/api/ir/learn/poll');
      if (r.ok) {
        await api('/api/ir/learn/stop', { method: 'POST' }).catch(() => {});
        if (msgEl) { msgEl.textContent = `Got it: ${r.protocol} ${r.code}`; msgEl.className = 'msg ok small'; }
        return r;
      }
    }
    throw new Error('Timeout — no IR signal received.');
  } finally {
    await api('/api/ir/learn/stop', { method: 'POST' }).catch(() => {});
  }
}

function populateCmdFileSelect() {
  const sel = $('cmd-file-select');
  if (!sel) return;
  sel.innerHTML = '';
  listPaths('Commands/').forEach((p) => {
    const o = document.createElement('option');
    o.value = p;
    o.textContent = p.replace('Commands/', '');
    sel.appendChild(o);
  });
  if (!selectedCmdFile) selectedCmdFile = commandFileForPage(selectedPagePath) || listPaths('Commands/')[0] || '';
  sel.value = selectedCmdFile;
  sel.onchange = () => { selectedCmdFile = sel.value; renderCommandsTable(); };
}

function escapeAttr(s) {
  return String(s).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;');
}

function renderCommandsTable() {
  selectedCmdFile = $('cmd-file-select')?.value || selectedCmdFile;
  const d = parseJson(selectedCmdFile) || { Commands: [] };
  const key = d.Commands ? 'Commands' : 'Actions';
  d[key] = d[key] || [];
  const tbody = $('cmd-table')?.querySelector('tbody');
  if (!tbody) return;
  tbody.innerHTML = '';
  d[key].forEach((cmd, idx) => {
    const tr = document.createElement('tr');
    const name = cmd.Command || cmd.Action || '';
    const mode = cmd.Mode || 'IR';
    const proto = cmd.Protocol || '';
    const data = Array.isArray(cmd.Data) ? cmd.Data.join(', ') : '';
    tr.innerHTML = `<td><input data-f="name" value="${escapeAttr(name)}" /></td>
      <td><select data-f="mode"><option${mode==='IR'?' selected':''}>IR</option><option${mode==='MQTT'?' selected':''}>MQTT</option></select></td>
      <td><input data-f="proto" value="${escapeAttr(proto)}" /></td>
      <td><input data-f="data" value="${escapeAttr(data)}" /></td>
      <td><button type="button" data-del="${idx}">×</button></td>`;
    tr.querySelectorAll('input,select').forEach((el) => {
      el.onchange = () => {
        const doc = parseJson(selectedCmdFile);
        const row = doc[key][idx];
        row.Command = row.Command || row.Action;
        if (key === 'Commands') {
          row.Command = tr.querySelector('[data-f="name"]').value;
          row.Mode = tr.querySelector('[data-f="mode"]').value;
          row.Protocol = tr.querySelector('[data-f="proto"]').value;
          row.Data = tr.querySelector('[data-f="data"]').value.split(',').map((s) => s.trim()).filter(Boolean);
        }
        setFile(selectedCmdFile, doc);
      };
    });
    tr.querySelector('[data-del]').onclick = () => {
      const doc = parseJson(selectedCmdFile);
      doc[key].splice(idx, 1);
      setFile(selectedCmdFile, doc);
      renderCommandsTable();
    };
    tbody.appendChild(tr);
  });
}

$('btn-add-command')?.addEventListener('click', () => {
  const doc = parseJson(selectedCmdFile) || { Commands: [] };
  doc.Commands.push({ Command: 'NEW', Mode: 'IR', Protocol: 'NEC', Data: ['0x0'] });
  setFile(selectedCmdFile, doc);
  renderCommandsTable();
});

$('btn-new-cmd-file')?.addEventListener('click', () => {
  const name = prompt('File name:', 'Commands/MyDevice.json');
  if (!name) return;
  const path = name.startsWith('Commands/') ? name : 'Commands/' + name;
  setFile(path, { Manufacturer: 'Custom', Commands: [] });
  selectedCmdFile = path;
  populateCmdFileSelect();
  renderCommandsTable();
});

$('btn-learn-ir')?.addEventListener('click', async () => {
  try {
    const cap = await learnIr($('learn-msg'));
    const doc = parseJson(selectedCmdFile) || { Commands: [] };
    doc.Commands.push({ Command: 'LEARNED_' + Date.now().toString(36).slice(-4), Mode: 'IR', Protocol: cap.protocol, Data: [cap.code.startsWith('0x') ? cap.code : '0x' + cap.code] });
    setFile(selectedCmdFile, doc);
    renderCommandsTable();
  } catch (e) {
    $('learn-msg').textContent = e.message;
    $('learn-msg').className = 'msg err';
  }
});

function populateRawSelect() {
  const sel = $('raw-file-select');
  if (!sel) return;
  sel.innerHTML = '';
  [...files.keys()].filter((p) => p.endsWith('.json')).sort().forEach((p) => {
    const o = document.createElement('option');
    o.value = p;
    o.textContent = p;
    sel.appendChild(o);
  });
  sel.onchange = () => {
    selectedRawFile = sel.value;
    $('raw-title').textContent = sel.value;
    $('raw-editor').value = files.get(sel.value)?.content || '';
  };
  if (!selectedRawFile) selectedRawFile = selectedPagePath || selectedScenePath || 'Scenes.json';
  sel.value = selectedRawFile;
  $('raw-editor').value = files.get(selectedRawFile)?.content || '';
};

$('btn-apply-raw')?.addEventListener('click', () => {
  const path = $('raw-file-select')?.value || selectedRawFile;
  if (!path) return;
  setFile(path, $('raw-editor').value);
  refreshAll();
});

$('btn-deploy').onclick = async () => {
  $('deploy-msg').textContent = 'Saving…';
  $('deploy-msg').className = 'msg';
  const dirty = [...files.entries()].filter(([, v]) => v.dirty);
  if (!dirty.length) {
    $('deploy-msg').textContent = 'No changes to save.';
    return;
  }
  try {
    const st = await api('/api/status').catch(() => null);
    if (st && !st.editor_sync) {
      try { await setEditorSyncMode(true); } catch { /* device may lack API until flash */ }
    }
    for (const [path, { content }] of dirty) {
      await api('/api/fs/write?path=' + encodeURIComponent(path), {
        method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: content, timeout: 30000
      });
    }
    const onlyHaSettings =
      dirty.length > 0 && dirty.every(([p]) => p === HA_SETTINGS_PATH);
    if (!onlyHaSettings) {
      await api('/api/device/reboot', { method: 'POST', timeout: 5000 }).catch(() => {});
      $('deploy-msg').textContent = `Saved ${dirty.length} file(s). Remote rebooting…`;
    } else {
      $('deploy-msg').textContent = `Saved HaSettings.json on remote (no reboot).`;
    }
    $('deploy-msg').className = 'msg ok';
    dirty.forEach(([p]) => { files.get(p).dirty = false; });
  } catch (e) {
    $('deploy-msg').textContent = e.message;
    $('deploy-msg').className = 'msg err';
  }
};

if (defaultApi().includes('.local') || defaultApi().match(/^http:\/\/192\.168\./)) {
  connectAndLoad().catch(() => {});
}
