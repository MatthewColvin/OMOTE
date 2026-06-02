/** Schema-driven device settings form (shared contract with firmware). */
(function (global) {
  const DEFAULT_DEVICE_SETTINGS_SCHEMA = {
    version: 1,
    title: 'Device settings',
    sections: [
      {
        id: 'sleep',
        title: 'Sleep & power',
        fields: [
          { key: 'display_timeout_ms', type: 'choice', label: 'Screen off after', default: 60000,
            options: [
              { label: '10 sec', value: 10000 }, { label: '1 min', value: 60000 },
              { label: '15 min', value: 900000 }
            ] },
          { key: 'motion_wake_enabled', type: 'boolean', label: 'Wake on motion', default: true },
          { key: 'key_wake_enabled', type: 'boolean', label: 'Wake on key press', default: true }
        ]
      }
    ]
  };

  function schemaFieldKeys(schema) {
    const keys = new Set();
    for (const section of schema?.sections || []) {
      for (const field of section.fields || []) {
        if (field?.key) keys.add(field.key);
      }
    }
    return keys;
  }

  function defaultsFromSchema(schema) {
    const out = {};
    for (const section of schema?.sections || []) {
      for (const field of section.fields || []) {
        if (field?.key && field.default !== undefined) out[field.key] = field.default;
      }
    }
    return out;
  }

  function readFieldValue(values, field) {
    const key = field.key;
    if (values && values[key] !== undefined && values[key] !== null) return values[key];
    return field.default;
  }

  function optionValues(field) {
    return (field.options || []).map((o) => String(o.value));
  }

  function appendStringOptions(select, field, current) {
    const cur = current == null ? '' : String(current);
    const values = optionValues(field);
    let matched = false;
    for (const opt of field.options || []) {
      const o = document.createElement('option');
      o.value = String(opt.value);
      o.textContent = opt.label ?? opt.value;
      if (o.value === cur) {
        o.selected = true;
        matched = true;
      }
      select.appendChild(o);
    }
    if (cur && !matched) {
      const o = document.createElement('option');
      o.value = cur;
      o.textContent = `Custom: ${cur}`;
      o.selected = true;
      select.appendChild(o);
    }
  }

  function renderField(container, field, values) {
    const v = { ...values };
    const wrap = document.createElement('label');
    wrap.className = field.type === 'boolean' ? 'checkbox-row field-block' : 'field-block';
    wrap.dataset.key = field.key;

    if (field.type === 'boolean') {
      const cb = document.createElement('input');
      cb.type = 'checkbox';
      cb.dataset.key = field.key;
      cb.checked = !!readFieldValue(v, field);
      wrap.appendChild(cb);
      wrap.append(' ' + (field.label || field.key));
    } else if (field.type === 'choice' && field.options?.length) {
      wrap.textContent = field.label || field.key;
      const sel = document.createElement('select');
      sel.dataset.key = field.key;
      const current = readFieldValue(v, field);
      for (const opt of field.options) {
        const o = document.createElement('option');
        o.value = String(opt.value);
        o.textContent = opt.label;
        if (Number(opt.value) === Number(current)) o.selected = true;
        sel.appendChild(o);
      }
      wrap.appendChild(sel);
    } else if (field.type === 'string' && field.options?.length) {
      wrap.textContent = field.label || field.key;
      const sel = document.createElement('select');
      sel.dataset.key = field.key;
      appendStringOptions(sel, field, readFieldValue(v, field));
      wrap.appendChild(sel);
    } else if (field.type === 'slider' || field.type === 'number') {
      wrap.textContent = field.label || field.key;
      const inp = document.createElement('input');
      inp.type = 'number';
      inp.dataset.key = field.key;
      if (field.min !== undefined) inp.min = field.min;
      if (field.max !== undefined) inp.max = field.max;
      inp.value = readFieldValue(v, field);
      wrap.appendChild(inp);
    } else if (field.type === 'string') {
      wrap.textContent = field.label || field.key;
      const inp = document.createElement('input');
      inp.type = 'text';
      inp.dataset.key = field.key;
      inp.value = readFieldValue(v, field) ?? '';
      wrap.appendChild(inp);
    }
    container.appendChild(wrap);
  }

  function renderSectionFields(container, section, values) {
    if (section.hint) {
      const p = document.createElement('p');
      p.className = 'muted small section-hint';
      p.textContent = section.hint;
      container.appendChild(p);
    }
    for (const field of section.fields || []) {
      renderField(container, field, values);
    }
  }

  function applyValuesToForm(container, values) {
    if (!container || !values) return;
    for (const [key, value] of Object.entries(values)) {
      const el = container.querySelector(`[data-key="${key}"]`);
      if (!el) continue;
      if (el.tagName === 'INPUT' && el.type === 'checkbox') {
        el.checked = !!value;
      } else {
        el.value = String(value);
      }
    }
  }

  let modalElements = null;

  function ensureModal() {
    if (modalElements) return modalElements;
    const modal = document.createElement('div');
    modal.id = 'device-settings-section-modal';
    modal.className = 'modal hidden';
    modal.setAttribute('role', 'dialog');
    modal.setAttribute('aria-modal', 'true');
    modal.innerHTML = `
      <div class="modal-panel device-settings-modal-panel">
        <h3 id="device-settings-modal-title" class="section-title"></h3>
        <p id="device-settings-modal-hint" class="muted small section-lead hidden"></p>
        <div id="device-settings-modal-body"></div>
        <div class="modal-actions">
          <button type="button" id="device-settings-modal-done" class="primary">Done</button>
        </div>
      </div>`;
    document.body.appendChild(modal);
    const done = modal.querySelector('#device-settings-modal-done');
    done.addEventListener('click', () => closeSectionModal());
    modal.addEventListener('click', (e) => {
      if (e.target === modal) closeSectionModal();
    });
    modalElements = {
      modal,
      title: modal.querySelector('#device-settings-modal-title'),
      hint: modal.querySelector('#device-settings-modal-hint'),
      body: modal.querySelector('#device-settings-modal-body'),
      rootContainer: null,
      schema: null,
    };
    return modalElements;
  }

  function openSectionModal(section, schema, rootContainer) {
    const ui = ensureModal();
    ui.rootContainer = rootContainer;
    ui.schema = schema;
    const values = collectDeviceSettingsFromForm(rootContainer, schema);
    ui.title.textContent = section.title || section.id || 'Settings';
    if (section.hint) {
      ui.hint.textContent = section.hint;
      ui.hint.classList.remove('hidden');
    } else {
      ui.hint.textContent = '';
      ui.hint.classList.add('hidden');
    }
    ui.body.innerHTML = '';
    renderSectionFields(ui.body, section, values);
    ui.modal.classList.remove('hidden');
  }

  function closeSectionModal() {
    if (!modalElements?.modal) return;
    const { body, rootContainer, schema, modal } = modalElements;
    if (rootContainer && schema) {
      const patch = collectDeviceSettingsFromForm(body, schema);
      applyValuesToForm(rootContainer, patch);
    }
    modal.classList.add('hidden');
  }

  function renderDeviceSettingsForm(container, schema, values) {
    if (!container) return;
    container.innerHTML = '';
    const s = schema || DEFAULT_DEVICE_SETTINGS_SCHEMA;
    const v = { ...defaultsFromSchema(s), ...(values || {}) };

    const toolbar = document.createElement('div');
    toolbar.className = 'row device-settings-toolbar';
    const expandBtn = document.createElement('button');
    expandBtn.type = 'button';
    expandBtn.className = 'small-btn';
    expandBtn.textContent = 'Expand all';
    expandBtn.addEventListener('click', () => {
      container.querySelectorAll('details.settings-section').forEach((d) => { d.open = true; });
    });
    const collapseBtn = document.createElement('button');
    collapseBtn.type = 'button';
    collapseBtn.className = 'small-btn';
    collapseBtn.textContent = 'Collapse all';
    collapseBtn.addEventListener('click', () => {
      container.querySelectorAll('details.settings-section').forEach((d) => { d.open = false; });
    });
    toolbar.appendChild(expandBtn);
    toolbar.appendChild(collapseBtn);
    container.appendChild(toolbar);

    let index = 0;
    for (const section of s.sections || []) {
      const details = document.createElement('details');
      details.className = 'settings-section';
      details.open = index === 0;
      index += 1;

      const summary = document.createElement('summary');
      summary.className = 'settings-section-summary';

      const titleSpan = document.createElement('span');
      titleSpan.className = 'settings-section-title';
      titleSpan.textContent = section.title || section.id || 'Settings';
      summary.appendChild(titleSpan);

      const fieldCount = (section.fields || []).length;
      if (fieldCount) {
        const badge = document.createElement('span');
        badge.className = 'settings-section-badge';
        badge.textContent = `${fieldCount} field${fieldCount === 1 ? '' : 's'}`;
        summary.appendChild(badge);
      }

      const popBtn = document.createElement('button');
      popBtn.type = 'button';
      popBtn.className = 'small-btn settings-section-popout';
      popBtn.textContent = 'Edit';
      popBtn.title = 'Open this section in a dialog';
      popBtn.addEventListener('click', (e) => {
        e.preventDefault();
        e.stopPropagation();
        openSectionModal(section, s, container);
      });
      summary.appendChild(popBtn);

      details.appendChild(summary);

      const body = document.createElement('div');
      body.className = 'settings-section-body';
      renderSectionFields(body, section, v);
      details.appendChild(body);

      container.appendChild(details);
    }
  }

  function collectDeviceSettingsFromForm(container, schema) {
    const out = { ...defaultsFromSchema(schema) };
    if (!container) return out;
    container.querySelectorAll('input[data-key], select[data-key]').forEach((el) => {
      const key = el.dataset.key;
      if (!key) return;
      if (el.tagName === 'INPUT' && el.type === 'checkbox') {
        out[key] = el.checked;
      } else if (el.tagName === 'SELECT') {
        const n = Number(el.value);
        out[key] = Number.isFinite(n) && el.value !== '' && String(n) === el.value ? n : el.value;
      } else if (el.tagName === 'INPUT') {
        const n = Number(el.value);
        out[key] = Number.isFinite(n) && el.type === 'number' ? n : el.value;
      }
    });
    return out;
  }

  global.OmoteSettingsForm = {
    DEFAULT_DEVICE_SETTINGS_SCHEMA,
    schemaFieldKeys,
    defaultsFromSchema,
    renderDeviceSettingsForm,
    collectDeviceSettingsFromForm,
    applyValuesToForm,
  };
})(typeof window !== 'undefined' ? window : globalThis);
