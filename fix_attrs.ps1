$path = "generate/interrupt_handlers.h"
$content = [System.IO.File]::ReadAllText($path)

# Remove 'interrupt' from all handlers so they compile as normal C functions
$content = $content.Replace("__attribute__((interrupt, used))", "__attribute__((used))")

# Add 'interrupt' back ONLY for the top-level exception handlers
$exceptions = "UndefinedInst", "SWI", "PREFETCH_ABORT", "DATA_ABORT", "IRQ", "FIQ"
foreach ($ex in $exceptions) {
    $target = "void INT_Excep_${ex}(void) __attribute__((used));"
    $replacement = "void INT_Excep_${ex}(void) __attribute__((interrupt, used));"
    $content = $content.Replace($target, $replacement)
}

[System.IO.File]::WriteAllText($path, $content)
Write-Host "Replaced interrupt attributes successfully."
