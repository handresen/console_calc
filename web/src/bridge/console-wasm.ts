export interface BindingStackEntry {
  level: number;
  display: string;
}

export interface BindingDefinitionEntry {
  name: string;
  expression: string;
}

export interface BindingConstantEntry {
  name: string;
  value: string;
}

export interface BindingFunctionEntry {
  name: string;
  arity_label: string;
  category: string;
  summary: string;
}

export interface BindingSnapshot {
  display_mode: string;
  max_stack_depth: number;
  stack: BindingStackEntry[];
  definitions: BindingDefinitionEntry[];
  constants: BindingConstantEntry[];
  functions: BindingFunctionEntry[];
}

export interface BindingEvent {
  kind: string;
  text: string;
  stack: BindingStackEntry[];
  definitions: BindingDefinitionEntry[];
  constants: BindingConstantEntry[];
  functions: BindingFunctionEntry[];
}

export interface BindingCommandResult {
  should_exit: boolean;
  events: BindingEvent[];
  snapshot: BindingSnapshot;
}

interface ConsoleCalcWasmModule {
  _console_calc_binding_session_create(): number;
  _console_calc_binding_session_destroy(session: number): void;
  _console_calc_binding_session_initialize(session: number): number;
  _console_calc_binding_session_submit(session: number, inputPtr: number): number;
  _console_calc_binding_session_last_result_json(session: number): number;
  _free(pointer: number): void;
  stringToNewUTF8(input: string): number;
  UTF8ToString(pointer: number): string;
}

interface ConsoleCalcWasmModuleFactory {
  (moduleArg?: {
    locateFile?: (path: string) => string;
    print?: (...args: unknown[]) => void;
    printErr?: (...args: unknown[]) => void;
  }): Promise<ConsoleCalcWasmModule>;
}

export class ConsoleWasmBridge {
  private modulePromise: Promise<ConsoleCalcWasmModule> | null = null;
  private sessionPointer: number | null = null;

  async initialize(): Promise<BindingCommandResult> {
    const module = await this.loadModule();
    if (this.sessionPointer === null) {
      this.sessionPointer = module._console_calc_binding_session_create();
      if (this.sessionPointer === 0) {
        throw new Error("Failed to create console_calc binding session");
      }
    }

    if (module._console_calc_binding_session_initialize(this.sessionPointer) !== 0) {
      throw new Error("Failed to initialize console_calc binding session");
    }

    return this.readLastResult(module);
  }

  async submit(input: string): Promise<BindingCommandResult> {
    const module = await this.loadModule();
    if (this.sessionPointer === null) {
      await this.initialize();
    }

    if (this.sessionPointer === null) {
      throw new Error("Binding session is not initialized");
    }

    const inputPointer = module.stringToNewUTF8(input);
    try {
      if (
        module._console_calc_binding_session_submit(this.sessionPointer, inputPointer) !== 0
      ) {
        throw new Error("Failed to submit command to console_calc binding session");
      }
    } finally {
      module._free(inputPointer);
    }

    return this.readLastResult(module);
  }

  dispose(): void {
    if (this.modulePromise === null || this.sessionPointer === null) {
      return;
    }

    void this.modulePromise.then((module) => {
      if (this.sessionPointer !== null) {
        module._console_calc_binding_session_destroy(this.sessionPointer);
        this.sessionPointer = null;
      }
    });
  }

  private async loadModule(): Promise<ConsoleCalcWasmModule> {
    if (this.modulePromise === null) {
      const moduleUrl = "/wasm/console_calc.mjs";
      this.modulePromise = import(/* @vite-ignore */ moduleUrl).then(
        async (module): Promise<ConsoleCalcWasmModule> => {
          const factory = module.default as ConsoleCalcWasmModuleFactory;
          return factory({
            locateFile: (filePath) => `/wasm/${filePath}`,
            print: () => undefined,
            printErr: () => undefined,
          });
        },
      );
    }

    return this.modulePromise;
  }

  private readLastResult(module: ConsoleCalcWasmModule): BindingCommandResult {
    if (this.sessionPointer === null) {
      throw new Error("Binding session is not initialized");
    }

    const resultPointer = module._console_calc_binding_session_last_result_json(
      this.sessionPointer,
    );
    return JSON.parse(module.UTF8ToString(resultPointer)) as BindingCommandResult;
  }
}
