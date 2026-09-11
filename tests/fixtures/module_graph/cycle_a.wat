(module
    (import
        "B"
        "b"
        (func $b
            (result i32)
        )
    )

    (func
        (export "a")
        (result i32)

        call $b
    )
)
