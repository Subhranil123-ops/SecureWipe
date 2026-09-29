import {
    PlayCircle,
    ExternalLink
} from "lucide-react";


const YOUTUBE_URL =
    "https://www.youtube.com/watch?v=0CUP6A33hT0";


const GITHUB_URL =
    "https://github.com/Subhranil123-ops/SecureWipe";


function ProjectResources() {

    return (
        <section
            className="
                mb-6
                overflow-hidden
                rounded-2xl
                border
                border-slate-200
                bg-white
                shadow-sm
            "
        >

            <div
                className="
                    flex
                    flex-col
                    gap-4
                    px-5
                    py-5
                    sm:px-6
                    lg:flex-row
                    lg:items-center
                    lg:justify-between
                "
            >

                {/* ==================================================
                    LEFT SIDE
                   ================================================== */}

                <div className="min-w-0">

                    <div
                        className="
                            flex
                            items-center
                            gap-2
                        "
                    >

                        <span
                            className="
                                inline-flex
                                h-8
                                w-8
                                items-center
                                justify-center
                                rounded-lg
                                bg-indigo-50
                                text-indigo-600
                            "
                        >
                            <PlayCircle
                                className="h-4 w-4"
                            />
                        </span>


                        <p
                            className="
                                text-[10px]
                                font-semibold
                                uppercase
                                tracking-[0.18em]
                                text-indigo-600
                            "
                        >
                            Project Resources
                        </p>

                    </div>


                    <h3
                        className="
                            mt-2
                            text-base
                            font-bold
                            tracking-tight
                            text-slate-900
                        "
                    >
                        Explore the SecureWipe demonstration
                    </h3>


                    <p
                        className="
                            mt-1
                            max-w-2xl
                            text-sm
                            leading-6
                            text-slate-500
                        "
                    >
                        Watch the complete project demonstration
                        or inspect the source code and
                        implementation repository.
                    </p>

                </div>


                {/* ==================================================
                    RIGHT SIDE — RESOURCE BUTTONS
                   ================================================== */}

                <div
                    className="
                        flex
                        flex-col
                        gap-3
                        sm:flex-row
                        sm:items-center
                    "
                >

                    {/* ==================================================
                        YOUTUBE
                       ================================================== */}

                    <a
                        href={YOUTUBE_URL}
                        target="_blank"
                        rel="noopener noreferrer"
                        aria-label="Watch SecureWipe demo on YouTube"
                        className="
                            inline-flex
                            items-center
                            justify-center
                            gap-2
                            rounded-xl
                            bg-red-600
                            px-4
                            py-3
                            text-sm
                            font-semibold
                            text-white
                            shadow-sm
                            transition
                            hover:bg-red-700
                            hover:-translate-y-0.5
                            focus:outline-none
                            focus:ring-2
                            focus:ring-red-500
                            focus:ring-offset-2
                        "
                    >

                        <PlayCircle
                            className="h-4 w-4"
                        />

                        Watch Demo

                        <ExternalLink
                            className="h-3.5 w-3.5"
                        />

                    </a>


                    {/* ==================================================
                        GITHUB
                       ================================================== */}

                    <a
                        href={GITHUB_URL}
                        target="_blank"
                        rel="noopener noreferrer"
                        aria-label="Open SecureWipe GitHub repository"
                        className="
                            inline-flex
                            items-center
                            justify-center
                            gap-2
                            rounded-xl
                            border
                            border-slate-300
                            bg-slate-950
                            px-4
                            py-3
                            text-sm
                            font-semibold
                            text-white
                            shadow-sm
                            transition
                            hover:bg-slate-800
                            hover:-translate-y-0.5
                            focus:outline-none
                            focus:ring-2
                            focus:ring-slate-500
                            focus:ring-offset-2
                        "
                    >

                        {/* GitHub SVG */}

                        <svg
                            viewBox="0 0 24 24"
                            className="h-4 w-4"
                            fill="currentColor"
                            aria-hidden="true"
                        >
                            <path
                                d="
                                    M12 2
                                    C6.477 2 2 6.477 2 12
                                    C2 16.42 4.865 20.17 8.84 21.49
                                    C9.34 21.58 9.52 21.27 9.52 21
                                    C9.52 20.76 9.51 20.13 9.51 19.29
                                    C6.73 19.89 6.14 17.95 6.14 17.95
                                    C5.68 16.8 5.03 16.5 5.03 16.5
                                    C4.12 15.88 5.1 15.89 5.1 15.89
                                    C6.1 15.96 6.63 16.91 6.63 16.91
                                    C7.52 18.43 8.97 18 9.54 17.74
                                    C9.63 17.09 9.89 16.65 10.17 16.4
                                    C7.95 16.15 5.62 15.29 5.62 11.18
                                    C5.62 10.01 6.04 9.05 6.73 8.3
                                    C6.62 8.04 6.25 6.95 6.84 5.48
                                    C6.84 5.48 7.73 5.2 9.5 6.4
                                    C10.35 6.17 11.25 6.05 12 6.05
                                    C12.75 6.05 13.65 6.17 14.5 6.4
                                    C16.27 5.2 17.16 5.48 17.16 5.48
                                    C17.75 6.95 17.38 8.04 17.27 8.3
                                    C17.96 9.05 18.38 10.01 18.38 11.18
                                    C18.38 15.3 16.05 16.14 13.83 16.39
                                    C14.18 16.69 14.48 17.28 14.48 18.19
                                    C14.48 19.5 14.47 20.56 14.47 21
                                    C14.47 21.27 14.65 21.58 15.15 21.49
                                    C19.135 20.17 22 16.42 22 12
                                    C22 6.477 17.523 2 12 2
                                    Z
                                "
                            />
                        </svg>


                        GitHub Repository


                        <ExternalLink
                            className="h-3.5 w-3.5"
                        />

                    </a>

                </div>

            </div>


            {/* ==================================================
                FOOTER STRIP
               ================================================== */}

            <div
                className="
                    border-t
                    border-slate-100
                    bg-slate-50
                    px-5
                    py-3
                    sm:px-6
                "
            >

                <div
                    className="
                        flex
                        flex-col
                        gap-2
                        text-xs
                        text-slate-500
                        sm:flex-row
                        sm:items-center
                        sm:justify-between
                    "
                >

                    <span>
                        SecureWipe · Secure data sanitization &
                        digital forensics
                    </span>


                    <span
                        className="
                            font-medium
                            text-slate-400
                        "
                    >
                        Demo & source available publicly
                    </span>

                </div>

            </div>

        </section>
    );
}


export default ProjectResources;