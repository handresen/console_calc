import "ol/ol.css";
import "./styles.css";

import { createApp } from "./app";

const root = document.querySelector<HTMLDivElement>("#app");

if (root === null) {
  throw new Error("Missing #app root");
}

createApp(root);
