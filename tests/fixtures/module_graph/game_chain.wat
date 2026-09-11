(module
    (import
        "physics"
        "step"
        (func $step
            (param i32)
            (result i32)
        )
    )

    (func
        (export "run")
        (result i32)

        i32.const 41
        call $step
    )
)
