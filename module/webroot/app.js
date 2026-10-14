(() => {
  const fields = [...document.querySelectorAll('[data-field]')];
  const status = document.querySelector('#status');
  const catalog = {
    pixel_7_pro: { BRAND: 'google', MANUFACTURER: 'Google', MODEL: 'Pixel 7 Pro', DEVICE: 'cheetah', PRODUCT: 'cheetah', FINGERPRINT: 'google/cheetah/cheetah:17/CP3A.260905.009/16091614:user/release-keys', RELEASE: '17', SDK_INT: '36' },
    pixel_8_pro: { BRAND: 'google', MANUFACTURER: 'Google', MODEL: 'Pixel 8 Pro', DEVICE: 'husky', PRODUCT: 'husky', FINGERPRINT: 'google/husky/husky:17/CP3A.260905.009/16091614:user/release-keys', RELEASE: '17', SDK_INT: '36' },
    pixel_9_pro: { BRAND: 'google', MANUFACTURER: 'Google', MODEL: 'Pixel 9 Pro', DEVICE: 'caiman', PRODUCT: 'caiman', FINGERPRINT: 'google/caiman/caiman:17/CP3A.260905.009/16091614:user/release-keys', RELEASE: '17', SDK_INT: '36' }
  };
  const readBuild = () => Object.fromEntries(fields.map((el) => [el.dataset.field, el.value.trim()]).filter(([, value]) => value));
  const writeBuild = (build) => fields.forEach((el) => { if (build[el.dataset.field] !== undefined) el.value = build[el.dataset.field]; });
  const readConfig = () => {
    const name = document.querySelector('#profileName').value.trim();
    const pkg = document.querySelector('#packageName').value.trim();
    const profiles = { [name]: { build: readBuild() } };
    const config = { default_profile: name, package_profiles: {}, profiles };
    if (pkg) config.package_profiles[pkg] = name;
    return config;
  };
  const validate = (config) => {
    const name = document.querySelector('#profileName').value.trim();
    const pkg = document.querySelector('#packageName').value.trim();
    if (!/^[A-Za-z][A-Za-z0-9_-]{0,63}$/.test(name)) throw new Error('Invalid profile name');
    if (pkg && !/^[A-Za-z][A-Za-z0-9_]*(\.[A-Za-z][A-Za-z0-9_]*)+$/.test(pkg)) throw new Error('Invalid package name');
    if (!config.profiles[name].build.MODEL) throw new Error('MODEL is required');
    const sdk = config.profiles[name].build.SDK_INT;
    if (sdk && (!/^\d{1,2}$/.test(sdk) || Number(sdk) < 21 || Number(sdk) > 36)) throw new Error('SDK_INT must be 21..36');
    return config;
  };
  document.querySelector('#deviceCatalog').onchange = (event) => {
    const key = event.target.value;
    if (!key) return;
    document.querySelector('#profileName').value = key;
    writeBuild(catalog[key]);
    status.textContent = `Loaded catalog profile: ${key}`;
  };
  document.querySelector('#validate').onclick = () => {
    try { validate(readConfig()); status.textContent = 'VALID: safe profile configuration'; }
    catch (error) { status.textContent = `INVALID: ${error.message}`; }
  };
  document.querySelector('#download').onclick = () => {
    try {
      const config = validate(readConfig());
      const blob = new Blob([JSON.stringify(config, null, 2)], { type: 'application/json' });
      const a = document.createElement('a'); a.href = URL.createObjectURL(blob); a.download = 'SkySpoof.json'; a.click();
      URL.revokeObjectURL(a.href); status.textContent = 'Downloaded SkySpoof.json';
    } catch (error) { status.textContent = `INVALID: ${error.message}`; }
  };
  document.querySelector('#reset').onclick = () => { window.location.reload(); };
})();
