# Debounce eagerly (report change immediately), keep per-key timers. We can use
# this because the kinT does not have to deal with noise.
DEBOUNCE_TYPE = asym_eager_defer_pk
