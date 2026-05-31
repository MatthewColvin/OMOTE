/* OMOTE firmware config editor — beginner-first, auto-linked JSON files */
const SCR_W = 240;
const SCR_H = 320;
const STATUS_H = 22;
const TAB_BAR_H = Math.round(SCR_H * 0.1);
const CONTENT_H = SCR_H - STATUS_H - TAB_BAR_H;
const ADVANCED_KEY = 'omote_editor_advanced';

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
let selectedActivityId = '';
let activeTabIdx = 0;
let selectedPagePath = '';
let selectedWidgetIdx = -1;
let selectedKeyName = '';
let selectedCmdFile = '';
let selectedRawFile = '';
let selection = { kind: null, widgetIdx: null, keyName: null };
let drag = null;

const $ = (id) => document.getElementById(id);

function sleep(ms) { return new Promise((r) => setTimeout(r, ms)); }

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
  if (!raw) return null;
  try { return JSON.parse(raw); } catch { return null; }
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

function resolvePagePath(fileName) {
  if (!fileName) return '';
  if (files.has(fileName)) return fileName;
  const inPages = 'Pages/' + fileName.replace(/^Pages\//, '');
  if (files.has(inPages)) return inPages;
  return fileName;
}

function sceneContext() {
  const scene = parseJson(selectedScenePath);
  const entry = scene?.Pages?.[activeTabIdx];
  const pagePath = entry ? resolvePagePath(entry.FileName) : selectedPagePath;
  return { scene, entry, pagePath, commandPrefix: entry?.CommandPrefix || '' };
}

function activityRegistry() {
  if (!files.has('Scenes.json')) return { Scenes: [] };
  return parseJson('Scenes.json') || { Scenes: [] };
}

function setActivityRegistry(reg) {
  setFile('Scenes.json', reg);
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

function addDeviceToScene(deviceName, templateKey = 'blank') {
  const slug = slugify(deviceName);
  const cmdPath = ensureCommandFile(slug, templateKey);
  const pagePath = ensurePageFile(slug, templateKey, cmdPath);
  const scene = parseJson(selectedScenePath) || { Type: 'Scene', Pages: [] };
  scene.Pages = scene.Pages || [];
  scene.Pages.push({
    PageName: deviceName,
    ShortName: deviceName.slice(0, 10),
    FileName: pagePath.replace(/^Pages\//, '')
  });
  if (!scene.ScreenName) scene.ScreenName = $('activity-name')?.value || 'My activity';
  setFile(selectedScenePath, scene);
  activeTabIdx = scene.Pages.length - 1;
  selectedPagePath = pagePath;
  refreshAll();
}

function addNewActivity(name) {
  const slug = slugify(name);
  const scenePath = `Scenes/Scene_${slug}.json`;
  setFile(scenePath, { Type: 'Scene', ScreenName: name, Pages: [] });
  const reg = activityRegistry();
  reg.Scenes = reg.Scenes || [];
  reg.Scenes.push({ SceneName: name, FileName: scenePath });
  setActivityRegistry(reg);
  selectedScenePath = scenePath;
  selectedActivityId = scenePath;
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
  if (name === 'remote') refreshRemoteTab();
  if (name === 'activities') refreshActivitiesTab();
  if (name === 'commands') renderCommandsTable();
  if (name === 'raw') populateRawSelect();
}

document.querySelectorAll('#nav button').forEach((b) => {
  b.onclick = () => showTab(b.dataset.tab);
});

$('advanced-mode').onchange = () => setAdvancedMode($('advanced-mode').checked);
$('device-url').value = defaultApi();
applyAdvancedMode();

async function connectAndLoad() {
  API = $('device-url').value.trim().replace(/\/$/, '') || 'http://omote.local';
  localStorage.setItem('omote_oo_api', API);
  $('connect-msg').textContent = 'Connecting…';
  $('connect-msg').className = 'msg';
  try {
    const st = await api('/api/status');
    $('status-bar').textContent = `${st.connected ? 'Connected' : 'Offline'} · ${st.ip || '?'} · ${st.hostname}.local`;
    const tree = await api('/api/fs/tree');
    files.clear();
    for (const p of tree.files || []) {
      const r = await api('/api/fs/read?path=' + encodeURIComponent(p));
      setFile(p, r.content, false);
    }
    if (!files.has('Scenes.json')) setFile('Scenes.json', { Scenes: [] }, false);
    initAfterLoad();
    $('connect-msg').textContent = `Loaded ${files.size} files. Start with My setup to add your devices.`;
    $('connect-msg').className = 'msg ok';
    showTab('activities');
  } catch (e) {
    $('connect-msg').textContent = e.message;
    $('connect-msg').className = 'msg err';
  }
}

$('btn-connect').onclick = connectAndLoad;

function initAfterLoad() {
  if (!selectedScenePath) {
    const reg = activityRegistry();
    if (reg.Scenes?.[0]?.FileName) selectedScenePath = reg.Scenes[0].FileName;
    else if (listPaths('Scenes/')[0]) selectedScenePath = listPaths('Scenes/')[0];
  }
  selectedActivityId = selectedScenePath;
  populateCmdFileSelect();
  refreshAll();
}

function refreshAll() {
  refreshActivitiesTab();
  refreshRemoteTab();
  populateRawSelect();
}

/* ── Activities (My setup) ── */
function refreshActivitiesTab() {
  const ul = $('activity-list');
  ul.innerHTML = '';
  const reg = activityRegistry();
  (reg.Scenes || []).forEach((a) => {
    const li = document.createElement('li');
    li.textContent = a.SceneName || a.FileName;
    li.className = a.FileName === selectedActivityId ? 'active' : '';
    li.onclick = () => {
      selectedActivityId = a.FileName;
      selectedScenePath = a.FileName;
      activeTabIdx = 0;
      refreshActivitiesTab();
      refreshRemoteTab();
    };
    ul.appendChild(li);
  });

  const hasSel = !!selectedScenePath && files.has(selectedScenePath);
  $('activity-empty').classList.toggle('hidden', hasSel);
  $('activity-editor').classList.toggle('hidden', !hasSel);
  if (!hasSel) return;

  const scene = parseJson(selectedScenePath);
  $('activity-title').textContent = scene?.ScreenName || 'Activity';
  $('activity-name').value = scene?.ScreenName || '';
  $('activity-screen-name').value = scene?.ScreenName || '';
  renderDeviceTabList(scene);
  renderCommandSequences(scene);
}

function renderDeviceTabList(scene) {
  const box = $('device-tab-list');
  box.innerHTML = '';
  (scene?.Pages || []).forEach((pg, idx) => {
    const row = document.createElement('div');
    row.className = 'device-tab-row';
    const name = document.createElement('input');
    name.value = pg.PageName || pg.ShortName || 'Device';
    name.placeholder = 'Device name (e.g. Living room TV)';
    const tab = document.createElement('input');
    tab.value = pg.ShortName || pg.PageName || '';
    tab.placeholder = 'Tab label on screen';
    const tpl = document.createElement('select');
    tpl.innerHTML = Object.entries(DEVICE_TEMPLATES).map(([k, v]) =>
      `<option value="${k}">${v.label || k}</option>`).join('');
    tpl.style.display = 'none';
    const configure = document.createElement('button');
    configure.type = 'button';
    configure.textContent = 'Configure';
    configure.onclick = () => {
      activeTabIdx = idx;
      selectedPagePath = resolvePagePath(pg.FileName);
      showTab('remote');
    };
    const del = document.createElement('button');
    del.type = 'button';
    del.textContent = '×';
    del.onclick = () => {
      const sc = parseJson(selectedScenePath);
      sc.Pages.splice(idx, 1);
      setFile(selectedScenePath, sc);
      activeTabIdx = Math.min(activeTabIdx, Math.max(0, sc.Pages.length - 1));
      refreshAll();
    };
    const saveRow = () => {
      const sc = parseJson(selectedScenePath);
      sc.Pages[idx].PageName = name.value;
      sc.Pages[idx].ShortName = tab.value || name.value.slice(0, 10);
      setFile(selectedScenePath, sc);
      refreshRemoteTab();
    };
    name.onchange = name.oninput = saveRow;
    tab.onchange = tab.oninput = saveRow;
    row.append(name, tab, configure, del);
    box.appendChild(row);
  });
}

$('activity-name').oninput = () => {
  const scene = parseJson(selectedScenePath) || {};
  scene.ScreenName = $('activity-name').value.trim();
  setFile(selectedScenePath, scene);
  const reg = activityRegistry();
  const hit = reg.Scenes?.find((s) => s.FileName === selectedScenePath);
  if (hit) hit.SceneName = scene.ScreenName;
  setActivityRegistry(reg);
  refreshActivitiesTab();
};

$('btn-new-activity').onclick = () => {
  const name = prompt('Activity name (shown on remote home screen):', 'Watch TV');
  if (!name?.trim()) return;
  addNewActivity(name.trim());
};

$('btn-add-device').onclick = () => {
  if (!selectedScenePath) return;
  const name = prompt('Device name for this tab:', 'Living room TV');
  if (!name?.trim()) return;
  const tpl = $('device-template')?.value || 'tv';
  addDeviceToScene(name.trim(), tpl);
};

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
  const reg = activityRegistry();
  (reg.Scenes || []).forEach((a) => {
    const o = document.createElement('option');
    o.value = a.FileName;
    o.textContent = a.SceneName || a.FileName;
    sel.appendChild(o);
  });
  sel.value = selectedScenePath || '';
  sel.onchange = () => {
    selectedScenePath = sel.value;
    selectedActivityId = sel.value;
    activeTabIdx = 0;
    refreshRemoteTab();
  };

  const ul = $('remote-device-list');
  ul.innerHTML = '';
  const scene = parseJson(selectedScenePath);
  (scene?.Pages || []).forEach((pg, i) => {
    const li = document.createElement('li');
    li.textContent = pg.ShortName || pg.PageName || `Device ${i + 1}`;
    li.className = i === activeTabIdx ? 'active' : '';
    li.onclick = () => {
      activeTabIdx = i;
      selectedPagePath = resolvePagePath(pg.FileName);
      clearSelection();
      refreshRemoteTab();
    };
    ul.appendChild(li);
  });

  const ctx = sceneContext();
  if (ctx.pagePath) selectedPagePath = ctx.pagePath;
  const entry = scene?.Pages?.[activeTabIdx];
  $('remote-device-label').textContent = entry?.PageName || entry?.ShortName || 'Device';
  const pg = currentPage();
  const hint = $('linked-files-hint');
  if (hint) hint.textContent = pg.CommandFile ? `Linked: ${selectedPagePath} → ${pg.CommandFile}` : '';
  renderWidgetList(pg);
  drawCanvas();
  renderRemoteKeymap();
  updateSelectionPanel();
}

function tabLabels() {
  const scene = parseJson(selectedScenePath);
  return (scene?.Pages || []).map((p, i) => p.ShortName || p.PageName || `Tab ${i + 1}`);
}

function renderRemoteKeymap() {
  const root = $('remote-keymap');
  if (!root) return;
  root.innerHTML = '';
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

  const powerRow = document.createElement('div');
  powerRow.className = 'remote-power-row';
  powerRow.appendChild(makeBtn('Power', 'Power', 'shape-power'));
  root.appendChild(powerRow);

  const screen = document.createElement('div');
  screen.className = 'remote-screen';
  screen.textContent = 'Touch screen (above)';
  root.appendChild(screen);

  const face = document.createElement('div');
  face.className = 'remote-face';

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
  root.appendChild(face);
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
    $('selection-title').textContent = 'Touch button';
    $('selection-sub').textContent = w?.Text || w?.Type || '';
    const hasCmd = !!(w?.Command && w.Type === 'Button');
    $('action-type').value = w?.Type === 'Title' || w?.Type === 'Label' ? 'widget_only' : (hasCmd ? 'ir_existing' : 'ir');
    $('action-touch-label').value = w?.Text || '';
    if ($('w-type')) $('w-type').value = w?.Type || 'Button';
    if ($('w-height')) $('w-height').value = w?.HeightPct ?? 10;
    if ($('w-posx')) $('w-posx').value = w?.PosX ?? '';
    if ($('w-posy')) $('w-posy').value = w?.PosY ?? '';
    populateActionCmdPick(Array.isArray(w?.Command) ? w.Command[0] : w?.Command);
  } else {
    $('selection-title').textContent = 'Physical key';
    $('selection-sub').textContent = friendlyLabel || KEY_LABELS[selection.keyName] || selection.keyName;
    const mapped = getKeyMapping(selection.keyName, $('key-press-type')?.value || 'Press');
    $('action-type').value = mapped ? 'ir_existing' : 'ir';
    populateActionCmdPick(mapped);
    $('action-cmd-name').value = DEFAULT_CMD_FOR_KEY[selection.keyName] || selection.keyName.toUpperCase();
  }
  syncActionPanels();
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
  const t = $('action-type').value;
  $('panel-ir').classList.toggle('hidden', t !== 'ir');
  $('panel-ir-existing').classList.toggle('hidden', t !== 'ir_existing');
  $('panel-touch-only').classList.toggle('hidden', t !== 'widget_only');
  $('panel-key-advanced').classList.toggle('hidden', selection.kind !== 'key');
}

$('action-type').onchange = syncActionPanels;

$('btn-action-apply').onclick = () => {
  const t = $('action-type').value;
  if (selection.kind === 'widget') {
    const page = currentPage();
    const w = page.Widgets?.[selection.widgetIdx];
    if (!w) return;
    if (t === 'widget_only') {
      w.Text = $('action-touch-label').value;
    } else if (t === 'ir_existing') {
      w.Command = $('action-cmd-pick').value;
      if (!w.Text) w.Text = $('action-cmd-pick').value;
    }
    w.Type = $('w-type')?.value || w.Type || 'Button';
    const h = parseInt($('w-height')?.value, 10);
    if (h) w.HeightPct = h;
    savePage(page);
  } else if (selection.kind === 'key') {
    const page = currentPage();
    page.ButtonMaps = page.ButtonMaps || {};
    const pt = $('key-press-type').value;
    const cmd = t === 'ir_existing' ? $('action-cmd-pick').value : $('action-cmd-name').value.trim();
    if (!cmd) return;
    page.ButtonMaps[selection.keyName] = page.ButtonMaps[selection.keyName] || {};
    page.ButtonMaps[selection.keyName][pt] = cmd;
    savePage(page);
  }
  refreshRemoteTab();
};

$('btn-action-clear').onclick = () => {
  if (selection.kind === 'widget') {
    const page = currentPage();
    const w = page.Widgets?.[selection.widgetIdx];
    if (w) delete w.Command;
    savePage(page);
  } else if (selection.kind === 'key') {
    const page = currentPage();
    const pt = $('key-press-type').value;
    if (page.ButtonMaps?.[selection.keyName]?.[pt]) {
      delete page.ButtonMaps[selection.keyName][pt];
      if (!Object.keys(page.ButtonMaps[selection.keyName]).length) delete page.ButtonMaps[selection.keyName];
    }
    savePage(page);
  }
  refreshRemoteTab();
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
    } else if (selection.kind === 'widget') {
      const page = currentPage();
      const w = page.Widgets[selection.widgetIdx];
      cmdName = (w?.Text || 'BTN').toUpperCase().replace(/\s+/g, '_');
      w.Command = cmdName;
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
  ul.innerHTML = '';
  (page?.Widgets || []).forEach((w, i) => {
    const li = document.createElement('li');
    const cmd = Array.isArray(w.Command) ? w.Command.join(',') : (w.Command || '');
    li.textContent = `${w.Text || w.Type}${cmd ? ' → ' + cmd : ''}`;
    li.className = selection.kind === 'widget' && selection.widgetIdx === i ? 'selected' : '';
    li.onclick = () => selectWidget(i);
    ul.appendChild(li);
  });
}

$('btn-add-button').onclick = () => {
  const page = currentPage();
  page.Widgets = page.Widgets || [];
  page.Widgets.push({ Type: 'Button', Text: 'New button', HeightPct: 10, AlignTo: page.Widgets.length });
  savePage(page);
  selectWidget(page.Widgets.length - 1);
};

$('btn-delete-widget').onclick = () => {
  if (selection.kind !== 'widget') return;
  const page = currentPage();
  page.Widgets.splice(selection.widgetIdx, 1);
  savePage(page);
  clearSelection();
  refreshRemoteTab();
};

/* ── Canvas ── */
function layoutFlowRects(widgets) {
  const rects = [];
  let y = STATUS_H + 4;
  (widgets || []).forEach((w, i) => {
    const hp = w.HeightPct || 10;
    const h = Math.round((CONTENT_H - 8) * hp / 100);
    if (w.PosX != null && w.PosY != null) {
      rects.push({
        i, x: Math.round(SCR_W * w.PosX / 100),
        y: STATUS_H + Math.round(CONTENT_H * w.PosY / 100),
        w: w.SizeXY ? Math.round(SCR_W * w.SizeXY[0] / 100) : SCR_W - 16,
        h: w.SizeXY ? Math.round(CONTENT_H * w.SizeXY[1] / 100) : h,
        widget: w
      });
    } else {
      rects.push({ i, x: 8, y, w: SCR_W - 16, h, widget: w });
      y += h + 4;
    }
  });
  return rects;
}

function drawCanvas() {
  const c = $('preview');
  if (!c) return;
  const ctx = c.getContext('2d');
  ctx.fillStyle = '#111';
  ctx.fillRect(0, 0, SCR_W, SCR_H);
  ctx.fillStyle = '#222';
  ctx.fillRect(0, 0, SCR_W, STATUS_H);
  ctx.fillStyle = '#aaa';
  ctx.font = '11px sans-serif';
  const scene = parseJson(selectedScenePath);
  ctx.fillText(scene?.ScreenName ? `Scene:${scene.ScreenName}` : 'OMOTE', 8, 15);

  const rects = layoutFlowRects(currentPage().Widgets || []);
  rects.forEach((r) => {
    const w = r.widget;
    const sel = selection.kind === 'widget' && selection.widgetIdx === r.i;
    ctx.strokeStyle = sel ? '#58a6ff' : '#555';
    ctx.lineWidth = sel ? 2 : 1;
    if (w.Type === 'ColorButtons') {
      ['#c0392b', '#27ae60', '#f1c40f', '#2980b9'].forEach((col, ci) => {
        ctx.fillStyle = col;
        ctx.fillRect(r.x + ci * (r.w / 4), r.y, r.w / 4 - 2, r.h);
      });
    } else {
      ctx.fillStyle = '#2a3548';
      ctx.fillRect(r.x, r.y, r.w, r.h);
      ctx.fillStyle = '#eee';
      ctx.font = '12px sans-serif';
      ctx.fillText((w.Text || w.Type || '').slice(0, 18), r.x + 6, r.y + r.h / 2 + 4);
    }
    ctx.strokeRect(r.x, r.y, r.w, r.h);
  });

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
}

function canvasHitTab(x, y) {
  if (y < SCR_H - TAB_BAR_H) return -1;
  const labels = tabLabels();
  if (!labels.length) return -1;
  return Math.min(labels.length - 1, Math.floor(x / (SCR_W / labels.length)));
}

$('preview').onmousedown = (ev) => {
  const c = $('preview');
  const rect = c.getBoundingClientRect();
  const x = (ev.clientX - rect.left) * (SCR_W / rect.width);
  const y = (ev.clientY - rect.top) * (SCR_H / rect.height);
  const tabHit = canvasHitTab(x, y);
  if (tabHit >= 0) {
    activeTabIdx = tabHit;
    selectedPagePath = resolvePagePath(parseJson(selectedScenePath)?.Pages?.[activeTabIdx]?.FileName);
    clearSelection();
    refreshRemoteTab();
    return;
  }
  if (y > SCR_H - TAB_BAR_H) return;
  const hit = layoutFlowRects(currentPage().Widgets || []).slice().reverse()
    .find((r) => x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
  if (!hit) { clearSelection(); return; }
  selectWidget(hit.i);
  if (hit.widget.Type === 'Button') drag = { idx: hit.i, ox: x - hit.x, oy: y - hit.y };
};

$('preview').onmousemove = (ev) => {
  if (!drag) return;
  const c = $('preview');
  const rect = c.getBoundingClientRect();
  const x = (ev.clientX - rect.left) * (SCR_W / rect.width);
  const y = (ev.clientY - rect.top) * (SCR_H / rect.height);
  const page = currentPage();
  const w = page.Widgets[drag.idx];
  w.PosX = Math.max(0, Math.min(100, Math.round((x - drag.ox) / SCR_W * 100)));
  w.PosY = Math.max(0, Math.min(100, Math.round((y - drag.oy - STATUS_H) / CONTENT_H * 100)));
  savePage(page);
  drawCanvas();
};
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
    for (const [path, { content }] of dirty) {
      await api('/api/fs/write?path=' + encodeURIComponent(path), {
        method: 'POST', headers: { 'Content-Type': 'text/plain' }, body: content, timeout: 30000
      });
    }
    await api('/api/device/reboot', { method: 'POST', timeout: 5000 }).catch(() => {});
    $('deploy-msg').textContent = `Saved ${dirty.length} file(s). Remote rebooting…`;
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
