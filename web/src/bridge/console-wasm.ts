export function createBridgePlaceholder(): HTMLElement {
  const banner = document.createElement("div");
  banner.className = "bridge-banner";
  banner.textContent =
    "WebAssembly bridge placeholder. Next step: load console_calc.mjs and connect the binding API.";
  return banner;
}
