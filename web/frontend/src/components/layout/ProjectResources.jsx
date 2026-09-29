import {
    Github,
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
                <div className="min-w-0">
                    <div className="flex items-center gap-2">
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
                            <PlayCircle className="h-4 w-4" />
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
                        Explore the ForenWipe demonstration
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
                        Watch the complete project demonstration or
                        inspect the source code and implementation
                        repository.
                    </p>
                </div>

                <div
                    className="
                        flex
                        flex-col
                        gap-3
                        sm:flex-row
                        sm:items-center
                    "
                >
                    <a
                        href={YOUTUBE_URL}
                        target="_blank"
                        rel="noopener noreferrer"
                        aria-label="Watch ForenWipe demo on YouTube"
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
                        <PlayCircle className="h-4 w-4" />

                        Watch Demo

                        <ExternalLink className="h-3.5 w-3.5" />
                    </a>

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
                        <Github className="h-4 w-4" />

                        GitHub Repository

                        <ExternalLink className="h-3.5 w-3.5" />
                    </a>
                </div>
            </div>

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
                        ForenWipe · Secure data sanitization & digital
                        forensics
                    </span>

                    <span className="font-medium text-slate-400">
                        Demo & source available publicly
                    </span>
                </div>
            </div>
        </section>
    );
}

export default ProjectResources;